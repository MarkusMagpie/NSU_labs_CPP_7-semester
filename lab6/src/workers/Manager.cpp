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
    StorageZone& storage) : id_(id), period_(period), cities_(cities), queue_(queue), dock_(dock), storage_(storage) {}

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
            Logger::log(who + " ставит задачу: инвентаризация (" + std::to_string(types.size()) + " вида товара)");
            
            return std::make_shared<InventoryTask>(types);
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

void Manager::run(std::stop_token stopToken) {
    Logger::log("Менеджер #" + std::to_string(id_) + " начал смену");

    auto gen = RandomGenerator::getGenerator();

    while (!stopToken.stop_requested()) {
        std::shared_ptr<Task> task = createRandomTask(gen, stopToken);
        if (task) {
            queue_.push(task); // push разбудит одного ждущего грузчика
        }
        std::this_thread::sleep_for(period_);
    }

    Logger::log("Менеджер #" + std::to_string(id_) + " закончил смену");
}
