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

TaskResult PickingTask::execute(Loader& loader) {
    // 1 - заказ от менеджера на комплектацию уже получен
    Logger::log(who(loader) + " комплектует заказ в город " 
        + std::to_string(order_.city)
        + " (до " + std::to_string(maxLoaders_) + " грузчиков)");

    // 2 - забронировать стол
    // 2a - не удалось за 5 единиц: грузчик пока один на задаче -> он последний -> задачу в очередь
    if (tableState_.table == -1 && !bookTable(loader)) {
        // bookTable вернул -1 и при остановке: тогда не "все столы заняты", а конец смены
        if (loader.stopRequested()) {
            return stopPicking(loader);
        }
        return TaskResult::Requeue;
    }

    bool waitingForPallets = false;
    while (true) {
        if (loader.stopRequested()) {
            return stopPicking(loader);
        }

        // паллеты отправки оставшиеся на столе с прошлого раза
        if (!shipReadyPallets(loader)) {
            // ship() прерывается и при остановке -> это не 5 неудачных попыток
            if (loader.stopRequested()) {
                return stopPicking(loader);
            }
            returnPallets(loader, true);

            // 3c ЗО: паллета отправки осталась на столе, стол забронирован -> задача со состоянием в очередь
            return TaskResult::Requeue;
        }

        std::optional<ProductType> type = currentType();
        // 9 - заказ больше не нуждается в комплектовке
        if (!type) {
            break;
        }

        // 3 - забрать паллеты хранения вида type на стол комплектации
        fetchPallets(loader, *type);
        if (tableState_.sortPallets.empty()) {
            // товар есть, но все его паллеты сейчас на других столах
            // или их заполняет разгрузка)
            if (!waitingForPallets) {
                std::ostringstream message;
                message << who(loader) 
                    << " ждет: свободных паллет вида " 
                    << *type << " сейчас нет";
                Logger::log(message.str());
                waitingForPallets = true;
            }
            std::this_thread::sleep_for(loader.actionTime());
            continue;
        }
        waitingForPallets = false;

        // 4-5
        bool shipped = moveUnits(loader, *type);
        if (!shipped && loader.stopRequested()) {
            return stopPicking(loader); // ship() прервала остановка
        }

        // шаги 6-7: паллеты этого вида опустели или больше не нужны заказу.
        // Если отправить не удалось - возвращаю все: паллеты хранения нужны и другим заказам
        returnPallets(loader, !shipped);
        if (!shipped) {
            return TaskResult::Requeue;
        }

        // 8 -> заказ еще нуждается в комплектовке -> повторить с 3
    }

    // 9а - вернуть паллеты хранения
    returnPallets(loader, true);

    // 9b - отменить бронирование стола (1 действие)
    std::this_thread::sleep_for(loader.actionTime());

    packing_.releaseTable(tableState_.table);
    Logger::log(who(loader) + " собрал и отправил заказ, стол " 
        + std::to_string(tableState_.table) + " освобожден");

    tableState_.table = -1;

    return TaskResult::Done;
}



std::string PickingTask::who(const Loader& loader) const {
    return "Грузчик #" + std::to_string(loader.id()) 
    + " [заказ #" + std::to_string(order_.id) + "]";
}

bool PickingTask::bookTable(Loader& loader) {
    // бронирование - 1 действие
    std::this_thread::sleep_for(loader.actionTime());

    // 2a - не дольше 5 единиц времени
    int table = packing_.bookTable(loader.stopToken(), timeUnit_ * limits::kTimeoutUnits);
    if (table < 0) {
        // при остановке лог напишет stopPicking
        if (!loader.stopRequested()) {
            Logger::log(who(loader) + " не смог забронировать стол комплектации: все заняты");
        }

        return false;
    }

    tableState_.table = table;

    Logger::log(who(loader) + " забронировал стол " 
        + std::to_string(table)
        + " (слотов для разбора: " 
        + std::to_string(packing_.sortSlots(table)) + ")");
    
    return true;
}

std::optional<ProductType> PickingTask::currentType() const {
    for (const auto& [type, count] : progress_.remaining) {
        if (count > 0) {
            return type;
        }
    }

    return std::nullopt;
}

// 3 - забрать паллеты хранения вида type на стол комплектации
void PickingTask::fetchPallets(Loader& loader, ProductType type) {
    // сколько единиц этого вида товара уже на столе комплектации 
    // в паллетах хранения
    int onTable = 0;
    for (StorageZone::CheckedOutPallet& pallet : tableState_.sortPallets) {
        if (pallet.type == type) {
            onTable += pallet.count;
        }
    }

    // 3 - не больше паллет чем вмещает стол + не больше чем нужно заказу
    while (static_cast<int>(tableState_.sortPallets.size()) < packing_.sortSlots(tableState_.table)
        && onTable < progress_.remaining.at(type) 
        && !loader.stopRequested()) {
        // если свободной паллеты нет то nullopt сразу (риск deadlock)
        std::optional<StorageZone::CheckedOutPallet> pallet = storage_.checkoutPallet(type);
        if (!pallet) break;

        // 3 - забрать паллету с полки
        std::this_thread::sleep_for(loader.actionTime());
        // 3a - отметить в терминале ЗУ что теперь в ЗУ
        std::this_thread::sleep_for(loader.actionTime());
        // 3b - отметить на столе комплектации что товар поступил для заказа 
        std::this_thread::sleep_for(loader.actionTime());

        tableState_.sortPallets.push_back(*pallet);
        onTable += pallet->count;

        std::ostringstream message;
        message << who(loader) << " принес на стол комплектации паллету " 
            << pallet->address
            << " (" << type << " x" << pallet->count << ")";
        Logger::log(message.str());
    }
}

bool PickingTask::moveUnits(Loader& loader, ProductType type) {
    int& remaining = progress_.remaining.at(type);

    for (StorageZone::CheckedOutPallet& sortPallet : tableState_.sortPallets) {
        if (sortPallet.type != type) {
            continue;
        }

        while (sortPallet.count > 0 && remaining > 0) {
            if (loader.stopRequested()) {
                return true;
            }

            // 4 - нахожу паллету отправки того же вида, незаполненную
            // или новую в свободном слоте упаковки
            int pal_index = packPalletFor(type);
            if (pal_index < 0) {
                Logger::log("ОШИБКА " + who(loader) + ": нет свободного слота для паллеты отправки");
                return false;
            }

            // 4 - переложить одну единицу = 1 действие
            std::this_thread::sleep_for(loader.actionTime());
            storage_.takeUnits(sortPallet.address, 1);
            --sortPallet.count;
            --remaining;
            ++tableState_.packPallets[pal_index].count;

            // 5 - паллета отправки заполнена -> в ЗО
            if (isReady(tableState_.packPallets[pal_index]) 
                && !shipPallet(loader, pal_index)) {
                return false;
            }
        }
    }
    return true;
}

int PickingTask::packPalletFor(ProductType type) {
    std::vector<Pallet>& pallets = tableState_.packPallets;

    // проверка совпадения типа и незаполненности 
    for (int i = 0; i < static_cast<int>(pallets.size()); ++i) {
        if (pallets[i].type == type && !isReady(pallets[i])) {
            return i;
        }
    }

    // есть ли на столе свободный слот для упаковки чтобы поставить 
    // новую пустую паллету отправки?
    if (static_cast<int>(pallets.size()) < packing_.packSlots(tableState_.table)) {
        pallets.push_back(Pallet{type, 0});
        
        return static_cast<int>(pallets.size()) - 1;
    }

    return -1;
}

bool PickingTask::isReady(const Pallet& pallet) const {
    // последняя паллета вида может быть неполной
    //  если больше этого товара заказу не нужно
    return pallet.count == palletCapacity(pallet.type) 
        || progress_.remaining.at(pallet.type) == 0;
}

// 5 - отметить готовность паллеты отправки index и отвезти в ЗО (ship)
bool PickingTask::shipPallet(Loader& loader, int index) {
    Pallet pallet = tableState_.packPallets[index];

    // 5a - отметить на столе что столько-то товара готово к отправке
    std::this_thread::sleep_for(loader.actionTime());

    // 5b - погрузка в ЗО (до 5 попыток найти машину в город заказа)
    ShippingZone::ShipResult result = shipping_.ship(order_.city, pallet, loader.actionTime(), timeUnit_, loader.stopToken());

    std::ostringstream message;
    message << who(loader) << ' ';
    if (!result.shipped && loader.stopRequested()) {
        // не дождался машину из-за конца смены, а не за 5 попыток
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

    tableState_.packPallets.erase(tableState_.packPallets.begin() + index);
    progress_.shipped[pallet.type] += pallet.count;
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

bool PickingTask::shipReadyPallets(Loader& loader) {
    for (int i = 0; i < static_cast<int>(tableState_.packPallets.size()); ) {
        if (!isReady(tableState_.packPallets[i])) {
            ++i;
            continue;
        }

        if (!shipPallet(loader, i)) {
            return false;
        }
    }

    return true;
}

// 9а - вернуть паллеты хранения со стола
void PickingTask::returnPallets(Loader& loader, bool all) {
    // паллеты хранения
    std::vector<StorageZone::CheckedOutPallet>& pallets = tableState_.sortPallets;

    auto act = [&loader] {
        if (!loader.stopRequested()) {
            std::this_thread::sleep_for(loader.actionTime());
        }
    };

    for (auto it = pallets.begin(); it != pallets.end(); ) {
        std::ostringstream message;
        message << who(loader) << ' ';

        if (it->count == 0) {
            // 6a - паллета опустела
            // отметить в терминале ЗУ что слот на полке освобожден
            act();

            message << "опустошил паллету " << it->address
                << ", место на полке освобождено";
        } else if (all || progress_.remaining.at(it->type) == 0) {
            // 7 - товар на паллете заказу больше не нужен
            // 7a - отвезти по исходному адресу
            act();
            // 7b - отметить в терминале ЗУ что паллета на месте
            act();
            
            message << "вернул паллету " << it->address 
                << " (" << it->type << " x" << it->count << ") на полку";
        } else {
            ++it;
            continue;
        }

        storage_.returnPallet(it->address);

        Logger::log(message.str());

        it = pallets.erase(it);
    }
}

TaskResult PickingTask::stopPicking(Loader& loader) {
    // 1) паллеты хранения со стола обратно на полки
    returnPallets(loader, true);

    // 2) бронь стола снять
    if (tableState_.table != -1) {
        packing_.releaseTable(tableState_.table);
    }

    // 3) резерв еще НЕ взятого товара свободен для других заказов.
    storage_.releaseReservation(progress_.remaining);

    // 4) собранные паллеты отправки остаются на столе
    int packedUnits = 0;
    for (const Pallet& pallet : tableState_.packPallets) {
        packedUnits += pallet.count;
    }

    Logger::log(who(loader) + " прервал комплектацию: смена закончилась"
        + (tableState_.table != -1 ? ", стол " + std::to_string(tableState_.table) + " освобожден" : "")
        + ", резерв снят; на паллетах отправки осталось " + std::to_string(packedUnits) + " шт");

    // задача больше не выполняется
    tableState_.table = -1;
    for (auto& [type, count] : progress_.remaining) {
        count = 0;
    }

    // смена закончилась -> в очередь не возвращаю
    return TaskResult::Done;
}

std::string PickingTask::name() const {
    return "Комплектация";
}

const Order& PickingTask::order() const {
    return order_;
}
