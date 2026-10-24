#include <string>
#include <vector>

#include <model/Order.h>
#include <model/Truck.h>
#include <tasks/InventoryTask.h>
#include <tasks/PickingTask.h>
#include <tasks/UnloadTask.h>
#include <utils/Logger.h>
#include <utils/RandomGenerator.h>
#include <workers/Manager.h>

// статическое поле объявлено в .h, создается здесь; нумерация с 1
std::atomic<int> Manager::nextId_{1};

Manager::Manager(int id,
    std::chrono::milliseconds period,
    int cities,
    TaskQueue& queue,
    ReceivingDock& dock,
    StorageZone& storage,
    ShippingZone& shipping,
    std::atomic<int>& tasksLeft) 
    : id_(id), 
    period_(period), 
    cities_(cities), 
    queue_(queue), 
    dock_(dock), 
    storage_(storage), 
    shipping_(shipping), 
    tasksLeft_(tasksLeft) {}

int Manager::id() const {
    return id_;
}

void Manager::start_thread() {
    // jthread создаст stop_token и передаст его в лямбду аргументом
    thread_ = std::jthread([this](std::stop_token stopToken) {
        run(stopToken);
    });
}

void Manager::request_stop() {
    thread_.request_stop();
}

void Manager::join() {
    // joinable() == false, если поток не запускали или уже дождались
    if (thread_.joinable()) {
        thread_.join();
    }
}

std::shared_ptr<Task> Manager::createRandomTask(std::mt19937& gen, std::stop_token stopToken) {
    std::string who = "Менеджер #" + std::to_string(id_);

    int number = nextId_++;

    switch (RandomGenerator::getRandom(gen, 0, 2)) {
        case 0: {
            // сначала машина должна встать в свободный слот приемки
            int slot = dock_.occupySlot(stopToken, period_);
            if (slot < 0) {
                Logger::log(who + ": машина #" + std::to_string(number)
                    + " не смогла встать на разгрузку так как все слоты зоны приемки заняты");
                return nullptr;
            }

            Truck truck = Truck::generate(number, gen);
            Logger::log(who + " ставит задачу: разгрузка машины #" 
                + std::to_string(number)
                + " в слоте " + std::to_string(slot)
                + " (" + std::to_string(truck.pallets.size()) 
                + " паллет, занято слотов: " + std::to_string(dock_.busyCount())
                + " из " + std::to_string(dock_.slotCount()) + ")");
            
            return std::make_shared<UnloadTask>(truck, slot, dock_, storage_);
        }
        case 1: {
            // случайный непустой набор видов товара
            std::vector<ProductType> types;
            for (ProductType type : kAllProductTypes) {
                if (RandomGenerator::getRandom(gen, 0, 1) == 1) {
                    types.push_back(type);
                }
            }
            if (types.empty()) {
                types.push_back(randomProductType(gen));
            }

            Logger::log(who + " ставит задачу: инвентаризация #" + std::to_string(number)
                + " (" + std::to_string(types.size()) + " вида товара)");

            auto task = std::make_shared<InventoryTask>(number, types, storage_);
            
            // отчеты придут когда грузчик выполнит задачу
            pendingReports_.emplace_back(number, task->getFuture());
            
            return task;
        }
        default: {
            Order order = Order::generate(number, cities_, gen);
            // сколько грузчиков могут одновременно комплектовать заказ: в ТЗ не задано!!! беру 1-3
            int maxLoaders = RandomGenerator::getRandom(gen, 1, 3);
            Logger::log(who + 
                " ставит задачу: комплектация заказа #" 
                + std::to_string(number)
                + " в город " + std::to_string(order.city));
            
            return std::make_shared<PickingTask>(order, maxLoaders);
        }
    }
}

void Manager::collectReadyReports() {
    std::string who = "Менеджер #" + std::to_string(id_);

    for (auto it = pendingReports_.begin(); it != pendingReports_.end(); ) {
        auto& [taskId, future] = *it;

        // готов ли результат
        if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            ++it;
            continue;
        }

        // результат уже готов -> get() забирает результат
        InventoryTask::Reports reports = future.get();

        for (const StorageZone::ProductReport& report : reports) {
            Logger::log(who + " получил отчет инвентаризации #" + std::to_string(taskId)
                + " - " + InventoryTask::describe(report));
        }

        it = pendingReports_.erase(it); // erase возвращает итератор на следующий элемент + удаление текущего
    }
}

void Manager::announceShippingTruck(std::mt19937& gen) {
    ShippingZone::ShippingTruck truck;
    truck.id = nextId_++;
    truck.city = RandomGenerator::getRandom(gen, 0, cities_ - 1);
    truck.capacity = RandomGenerator::getRandom(gen, limits::kMinShippingTruckPlaces, limits::kMaxShippingTruckPlaces);

    // если все слоты заняты то машина не встает (stop_token не нужен; ожидания нет)
    int slot = shipping_.arrive(truck, std::stop_token{}, std::chrono::milliseconds(0));

    std::string who = "Менеджер #" + std::to_string(id_);
    if (slot < 0) {
        Logger::log(who + ": машина для отправки #" 
            + std::to_string(truck.id)
            + " не смогла встать так как все слоты зоны отправки заняты");
        
        return;
    }

    Logger::log(who + " оповещает: машина для отправки #" 
        + std::to_string(truck.id)
        + " в город " + std::to_string(truck.city)
        + " встала в слот " + std::to_string(slot)
        + " (" + std::to_string(truck.capacity) 
        + " мест, занято слотов: "
        + std::to_string(shipping_.busyCount()) 
        + " из " + std::to_string(shipping_.slotCount()) + ")");
}

void Manager::run(std::stop_token stopToken) {
    Logger::log("Менеджер #" + std::to_string(id_) + " начал смену");

    auto gen = RandomGenerator::getGenerator();
    bool limitReported = false;

    while (!stopToken.stop_requested()) {
        // std::shared_ptr<Task> task = createRandomTask(gen, stopToken);
        // if (task) {
        //     queue_.push(task); // push разбудит одного ждущего грузчика
        // }
        
        if (tasksLeft_.fetch_sub(1) > 0) { // успел взять единицу из лимита
            std::shared_ptr<Task> task = createRandomTask(gen, stopToken);
            if (task) {
                queue_.push(task);
            } else {
                ++tasksLeft_; // задачу поставить не вышло (нет слота) -> вернуть единицу
            }
        } else {
            // fetch_sub уменьшил счетчик и при неудачной попытке -> вернуть, иначе единица потеряется:
            // другой менеджер мог вернуть свою единицу (нет слота) и она ушла бы в "минус"
            ++tasksLeft_;

            if (!limitReported) {
                Logger::log("Менеджер #" + std::to_string(id_) + ": лимит задач на смену исчерпан, новые задачи не ставлю");
                limitReported = true;
            }
        }
        // оповещение о приходе машины для отправки: не задача для грузчиков, на лимит задач не влияет
        if (RandomGenerator::getRandom(gen, 0, 2) == 0) {
            announceShippingTruck(gen);
        }
        collectReadyReports(); // для таски инвентаризации
        std::this_thread::sleep_for(period_);
    }

    collectReadyReports();
    if (!pendingReports_.empty()) {
        Logger::log("Менеджер #" + std::to_string(id_) + " не дождался отчетов инвентаризации: "
            + std::to_string(pendingReports_.size()));
    }

    Logger::log("Менеджер #" + std::to_string(id_) + " закончил смену");
}
