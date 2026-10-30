#include <algorithm>
#include <sstream>
#include <string>
#include <thread>

#include <tasks/PickingTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

PickingTask::PickingTask(const Order& order, int maxLoaders,
    StorageZone& storage, PackingZone& packing, ShippingZone& shipping,
    std::chrono::milliseconds timeUnit)
    : order_(order),
    maxLoaders_(maxLoaders),
    storage_(storage),
    packing_(packing),
    shipping_(shipping),
    timeUnit_(timeUnit),
    progress_{order.items, {}} {}

// одновременно работают до maxLoaders_ грузчиков, у всех один цикл:
//     под state_mutex_ решить что делать и изменить состояние стола ->
//     отпустить мьютекс -> выполнить долгую часть (sleep / ship / bookTable)
// так грузчики не ждут друг друга пока кто-то спит или стоит в зоне отправки
TaskResult PickingTask::execute(Loader& loader) {
    // 1 - задача получена; встать на нее если есть место
    if (!tryJoin(loader)) {
        return TaskResult::Done;
    }

    // ожидание = 1 действие; в лог жижу только когда причина меняется, иначе спам
    std::string lastWait;
    auto wait = [&](const std::string& reason) {
        if (reason != lastWait) {
            Logger::log(who(loader) + " ждет: " + reason);
            lastWait = reason;
        }

        std::this_thread::sleep_for(loader.actionTime());
    };

    while (true) {
        if (loader.stopRequested()) {
            return leave(loader, LeaveReason::Stop);
        }

        std::unique_lock<std::mutex> lock(state_mutex_);

        // 2 - стол еще не забронирован
        if (tableState_.table == -1) {
            if (tableState_.booking) {
                lock.unlock();
                wait("другой грузчик бронирует стол");
                continue;
            }
            tableState_.booking = true; // бронирую я, а остальные будут ждать
            lock.unlock();
            lastWait.clear();

            if (!bookTable(loader)) {
                // 2a - не дали стол за 5 единиц: не последний -> отказ, последний -> в очередь
                return leave(loader, loader.stopRequested() ? LeaveReason::Stop : LeaveReason::Failed);
            }
            continue;
        }

        // 5 - на столе есть готовая к отправке паллета отправки которую никто не везет
        // (заполнил я или другой лодер, или она осталась после неудачной попытки? а хуй его знает)
        if (std::optional<PackPallet> ready = claimReadyPackPalletLocked()) {
            lock.unlock();
            lastWait.clear();

            if (!shipPallet(loader, *ready)) {
                // 3c ЗО: паллета снова на столе; не последний -> отказ, последний -> в очередь
                return leave(loader, loader.stopRequested() ? LeaveReason::Stop : LeaveReason::Failed);
            }
            continue;
        }

        // 6-7 - паллета хранения пустая или больше не нужна -> 
        // освободить слот разбора
        if (std::optional<StorageZone::CheckedOutPallet> useless = takeUselessSortPalletLocked()) {
            lock.unlock();
            lastWait.clear();

            // возврат паллеты на полку в ЗХ
            returnPallet(loader, *useless);
            continue;
        }

        // 9 - собирать больше нечего (последние паллеты, возможно, еще везут коллеги)
        std::optional<ProductType> type = currentTypeLocked();
        if (!type) {
            lock.unlock();
            
            return leave(loader, LeaveReason::NoWork);
        }

        // 4 - переложить одну единицу товара
        MoveResult moved = moveUnitLocked(*type);
        if (moved == MoveResult::Moved) {
            lock.unlock();
            lastWait.clear();
            std::this_thread::sleep_for(loader.actionTime()); // 1 единица = 1 действие
            continue;
        }
        if (moved == MoveResult::NoPackSlot) {
            lock.unlock();
            wait("оба слота упаковки заняты паллетами, которые везут в зону отправки");
            continue;
        }

        // 3 - на столе нет товара этого вида -> принести паллету хранения из ЗХ
        std::string waitReason;
        if (std::optional<StorageZone::CheckedOutPallet> pallet = startFetchLocked(*type, waitReason)) {
            lock.unlock();
            lastWait.clear();

            finishFetch(loader, *pallet);
            continue;
        }

        // 8 - работы на столе сейчас нет, но заказ не собран -> подождать и повторить
        lock.unlock();
        wait(waitReason);
    }
}

std::string PickingTask::who(const Loader& loader) const {
    return "Грузчик #" + std::to_string(loader.id())
    + " [заказ #" + std::to_string(order_.id) + "]";
}

bool PickingTask::needsMoreLoaders() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (finished_ || activeLoaders_ + 1 >= maxLoaders_) {
        return false;
    }

    return currentTypeLocked().has_value(); // есть что собирать
}

bool PickingTask::tryJoin(Loader& loader) {
    std::string message;
    bool joined = false;
    {
        // проверка и ++ - одной операцией под мьютексом: иначе двое одновременно увидят
        // "есть место" и грузчиков станет больше maxLoaders_ (как truckId в ShippingZone::load)
        std::lock_guard<std::mutex> lock(state_mutex_);

        if (finished_) {
            message = " отказывается: заказ уже собран или прерван";
        } else if (activeLoaders_ >= maxLoaders_) {
            message = " отказывается: на задаче уже " + std::to_string(activeLoaders_) + " грузчиков (максимум)";
        } else {
            ++activeLoaders_;
            joined = true;
            message = " комплектует заказ в город " + std::to_string(order_.city)
                + " (грузчиков на задаче: " + std::to_string(activeLoaders_)
                + " из " + std::to_string(maxLoaders_) + ")";
        }
    }

    Logger::log(who(loader) + message);
    return joined;
}

TaskResult PickingTask::leave(Loader& loader, LeaveReason reason) {
    std::unique_lock<std::mutex> lock(state_mutex_);

    int left = --activeLoaders_;
    if (left > 0) {
        // ТЗ 2a / 3c: "отказывается от задачи, если он не последний" - остальные продолжат
        // уборку стола, резерва сделает тот кто уйдет последним
        lock.unlock();
        Logger::log(who(loader) + " уходит с задачи, на ней остаются грузчиков: " + std::to_string(left));
        return TaskResult::Done;
    }

    // Я ПОСЛЕДНИЙ. Пока я был на задаче, никто другой не мог стать последним -> уборка только здесь
    if (reason == LeaveReason::NoWork) {
        // 9 - заказ собран: паллеты хранения уже вернули по шагам 6-7, паллеты отправки уехали
        finished_ = true;
        int table = tableState_.table;
        tableState_.table = -1;
        lock.unlock();

        // 9b - отменить бронирование стола
        std::this_thread::sleep_for(loader.actionTime());
        packing_.releaseTable(table);
        Logger::log(who(loader) + " собрал и отправил заказ, стол " + std::to_string(table) + " освобожден");
        return TaskResult::Done;
    }

    // Failed / Stop: паллеты хранения снимаю со стола под мьютексом, а возвращаю на полки уже без него
    std::vector<StorageZone::CheckedOutPallet> pallets = std::move(tableState_.sortPallets);
    tableState_.sortPallets.clear();

    if (reason == LeaveReason::Failed) {
        // ТЗ 2a / 3c: последний -> задачу в очередь со всем состоянием (бронь стола, паллеты отправки).
        // Паллеты хранения возвращаю: ждущая в очереди задача не должна держать товар других заказов
        lock.unlock();
        for (const StorageZone::CheckedOutPallet& pallet : pallets) {
            returnPallet(loader, pallet);
        }
        Logger::log(who(loader) + " последний на задаче, возвращает ее в очередь");
        return TaskResult::Requeue;
    }

    // Stop: смена закончилась посреди заказа -> убрать за собой
    finished_ = true;
    int table = tableState_.table;
    tableState_.table = -1;
    // резерв только еще НЕ взятого товара: уже переложенное снято с полок (takeUnits)
    std::map<ProductType, int> notTaken = progress_.remaining;
    for (auto& [type, count] : progress_.remaining) {
        count = 0;
    }
    // собранные паллеты отправки остаются на столе: товар на них в учете "в упаковке" -> баланс сходится
    int packedUnits = 0;
    for (const PackPallet& packPallet : tableState_.packPallets) {
        packedUnits += packPallet.pallet.count;
    }
    lock.unlock();

    for (const StorageZone::CheckedOutPallet& pallet : pallets) {
        returnPallet(loader, pallet);
    }
    if (table != -1) {
        packing_.releaseTable(table);
    }
    storage_.releaseReservation(notTaken);

    Logger::log(who(loader) + " прервал комплектацию: смена закончилась"
        + (table != -1 ? ", стол " + std::to_string(table) + " освобожден" : "")
        + ", резерв снят; на паллетах отправки осталось " + std::to_string(packedUnits) + " шт");
    
    return TaskResult::Done;
}

bool PickingTask::bookTable(Loader& loader) {
    // бронирование - 1 действие
    std::this_thread::sleep_for(loader.actionTime());

    // 2a - не дольше 5 единиц времени
    int table = packing_.bookTable(loader.stopToken(), timeUnit_ * limits::kTimeoutUnits);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        tableState_.booking = false;
        if (table >= 0) {
            tableState_.table = table;
        }
    }

    if (table < 0) {
        // при остановке лог напишет leave
        if (!loader.stopRequested()) {
            Logger::log(who(loader) + " не смог забронировать стол комплектации: все заняты");
        }
        return false;
    }

    Logger::log(who(loader) + " забронировал стол "
        + std::to_string(table)
        + " (слотов для разбора: "
        + std::to_string(packing_.sortSlots(table)) + ")");

    return true;
}

std::optional<ProductType> PickingTask::currentTypeLocked() const {
    // виды собираются по очереди: все грузчики работают над одним видом
    // беру первый вид которого не хватает
    for (const auto& [type, count] : progress_.remaining) {
        if (count > 0) {
            return type;
        }
    }

    return std::nullopt;
}

std::optional<StorageZone::CheckedOutPallet> PickingTask::startFetchLocked(ProductType type, std::string& waitReason) {
    // 3 - не больше паллет чем вмещает стол (с учетом тех что уже несут)
    int busySlots = static_cast<int>(tableState_.sortPallets.size()) 
        + tableState_.incomingPallets;
    if (busySlots >= packing_.sortSlots(tableState_.table)) {
        waitReason = "все слоты для разбора заняты";

        return std::nullopt;
    }

    // ... и не больше товара, чем нужно заказу (с учетом того, что уже несут коллеги)
    int onTable = tableState_.incomingUnits[type];
    for (const StorageZone::CheckedOutPallet& pallet : tableState_.sortPallets) {
        if (pallet.type == type) {
            onTable += pallet.count;
        }
    }
    if (onTable >= progress_.remaining.at(type)) {
        std::ostringstream message;
        message << "нужный товар вида " << type << " уже несут на стол";
        waitReason = message.str();

        return std::nullopt;
    }

    // не ждет: свободной паллеты нет -> nullopt сразу (ждать под state_mutex_ = остановить всех коллег)
    std::optional<StorageZone::CheckedOutPallet> pallet = storage_.checkoutPallet(type);
    if (!pallet) {
        std::ostringstream message;
        message << "свободных паллет вида " << type << " сейчас нет";
        waitReason = message.str();

        return std::nullopt;
    }

    // слот разбора и товар паллеты уже за нами, хотя паллету еще несут
    ++tableState_.incomingPallets;
    tableState_.incomingUnits[type] += pallet->count;

    return pallet;
}

void PickingTask::finishFetch(Loader& loader, const StorageZone::CheckedOutPallet& pallet) {
    // 3 - забрать паллету с полки
    std::this_thread::sleep_for(loader.actionTime());
    // 3a - отметить в терминале ЗУ что теперь в ЗУ
    std::this_thread::sleep_for(loader.actionTime());
    // 3b - отметить на столе комплектации что товар поступил для заказа
    std::this_thread::sleep_for(loader.actionTime());

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        tableState_.sortPallets.push_back(pallet); // палета теперь на столе
        --tableState_.incomingPallets;
        tableState_.incomingUnits[pallet.type] -= pallet.count;
    }

    std::ostringstream message;
    message << who(loader) << " принес на стол комплектации паллету "
        << pallet.address << " (" << pallet.type << " x" << pallet.count << ")";
    Logger::log(message.str());
}

PickingTask::MoveResult PickingTask::moveUnitLocked(ProductType type) {
    auto sortPallet = std::find_if(tableState_.sortPallets.begin(), 
        tableState_.sortPallets.end(),
        [type](const StorageZone::CheckedOutPallet& pallet) {
            return pallet.type == type && pallet.count > 0;
        });

    if (sortPallet == tableState_.sortPallets.end()) {
        return MoveResult::NoUnits;
    }

    // 4 - паллета отправки того же вида (незаполненная) или новая в свободном слоте упаковки
    int index = packPalletForLocked(type);
    if (index < 0) {
        return MoveResult::NoPackSlot;
    }

    storage_.takeUnits(sortPallet->address, 1);
    --sortPallet->count;
    --progress_.remaining.at(type);
    ++tableState_.packPallets[index].pallet.count;

    return MoveResult::Moved;
}

int PickingTask::packPalletForLocked(ProductType type) {
    std::vector<PackPallet>& pallets = tableState_.packPallets;

    // того же вида, не готовая и ее никто не везет
    for (int i = 0; i < static_cast<int>(pallets.size()); ++i) {
        if (pallets[i].pallet.type == type && !pallets[i].shipping && !isReadyLocked(pallets[i].pallet)) {
            return i;
        }
    }

    // 2 слота упаковки: пока одну паллету везут в ЗО, на второй коллеги продолжают собирать
    if (static_cast<int>(pallets.size()) < packing_.packSlots(tableState_.table)) {
        pallets.push_back(PackPallet{tableState_.nextPackId++, Pallet{type, 0}, false});
        return static_cast<int>(pallets.size()) - 1;
    }

    return -1;
}

bool PickingTask::isReadyLocked(const Pallet& pallet) const {
    // последняя паллета вида может быть неполной
    //  если больше этого товара заказу не нужно
    return pallet.count == palletCapacity(pallet.type)
        || progress_.remaining.at(pallet.type) == 0;
}

std::optional<PickingTask::PackPallet> PickingTask::claimReadyPackPalletLocked() {
    for (PackPallet& packPallet : tableState_.packPallets) {
        if (!packPallet.shipping && isReadyLocked(packPallet.pallet)) {
            packPallet.shipping = true; // везу я: другой грузчик эту паллету уже не возьмет
            
            return packPallet;
        }
    }

    return std::nullopt;
}

// 5 - отметить готовность паллеты отправки и отвезти в ЗО (ship)
bool PickingTask::shipPallet(Loader& loader, const PackPallet& packPallet) {
    const Pallet& pallet = packPallet.pallet;

    // 5a - отметить на столе что столько-то товара готово к отправке
    std::this_thread::sleep_for(loader.actionTime());

    // 5b - погрузка в ЗО (до 5 попыток найти машину в город заказа); state_mutex_ НЕ захвачен
    ShippingZone::ShipResult result = shipping_.ship(order_.city, pallet, loader.actionTime(), timeUnit_, loader.stopToken());

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = std::find_if(tableState_.packPallets.begin(), tableState_.packPallets.end(),
            [&packPallet](const PackPallet& p) { return p.id == packPallet.id; });

        if (result.shipped) {
            tableState_.packPallets.erase(it); // слот упаковки свободен
            progress_.shipped[pallet.type] += pallet.count;
        } else {
            it->shipping = false; // 3c ЗО: паллета снова на столе, ее может отвезти другой грузчик
        }
    }

    std::ostringstream message;
    message << who(loader) << ' ';
    if (!result.shipped && loader.stopRequested()) {
        message << "прервал отправку паллеты (" << pallet << "): смена закончилась, паллета осталась на столе";
        Logger::log(message.str());
        return false;
    }
    if (!result.shipped) {
        message << "не смог отправить паллету отправки (" << pallet
            << ") за " << result.attempts
            << " попыток: нет машины в город "
            << order_.city << ", паллета возвращена на стол";
        Logger::log(message.str());
        return false;
    }

    message << "погрузил паллету отправки (" << pallet << ") в машину, попыток: " << result.attempts;

    // машина заполнилась и уехала -> ее груз (все заказы в ней) отправлен
    if (result.departedTruck) {
        storage_.registerShipped(result.departedTruck->cargo);
        message << "; машина #" << result.departedTruck->id << " заполнена и уехала в город "
            << result.departedTruck->city;
    }
    Logger::log(message.str());
    return true;
}

std::optional<StorageZone::CheckedOutPallet> PickingTask::takeUselessSortPalletLocked() {
    std::vector<StorageZone::CheckedOutPallet>& pallets = tableState_.sortPallets;

    auto it = std::find_if(pallets.begin(), pallets.end(),
        [this](const StorageZone::CheckedOutPallet& pallet) {
            return pallet.count == 0 || progress_.remaining.at(pallet.type) == 0;
        });

    if (it == pallets.end()) {
        return std::nullopt;
    }

    // снимаю со стола под мьютексом -> никто уже не возьмет с нее единицу, пока я ее везу
    StorageZone::CheckedOutPallet pallet = *it;
    pallets.erase(it);

    return pallet;
}

void PickingTask::returnPallet(Loader& loader, const StorageZone::CheckedOutPallet& pallet) {
    // при остановке учет закрывается сразу, без действий: иначе потоки завершались бы долго
    auto act = [&loader] {
        if (!loader.stopRequested()) {
            std::this_thread::sleep_for(loader.actionTime());
        }
    };

    std::ostringstream message;
    message << who(loader) << ' ';

    if (pallet.count == 0) {
        // 6a - паллета опустела: отметить в терминале ЗУ, слот на полке освобождается
        act();
        message << "опустошил паллету " << pallet.address << ", место на полке освобождено";
    } else {
        // 7a - отвезти по исходному адресу; 7b - отметить в терминале ЗУ что паллета на месте
        act();
        act();
        message << "вернул паллету " << pallet.address
            << " (" << pallet.type << " x" << pallet.count << ") на полку";
    }

    storage_.returnPallet(pallet.address);
    Logger::log(message.str());
}

std::string PickingTask::name() const {
    return "Комплектация";
}

const Order& PickingTask::order() const {
    return order_;
}
