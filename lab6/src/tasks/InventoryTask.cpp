#include <sstream>
#include <string>
#include <thread>

#include <tasks/InventoryTask.h>
#include <utils/Logger.h>
#include <workers/Loader.h>

InventoryTask::InventoryTask(int id, const std::vector<ProductType>& types, StorageZone& storage)
    : id_(id), types_(types), storage_(storage) {}

void InventoryTask::execute(Loader& loader) {
    std::string who = "Грузчик #" + std::to_string(loader.id());
    std::string taskName = "инвентаризацию #" + std::to_string(id_);

    // exchange: поставить true и вернуть прежнее значение
    if (taken_.exchange(true)) {
        Logger::log(who + ": " + taskName + " уже выполняет другой грузчик, отказываюсь");
        return;
    }

    std::ostringstream start;
    start << who << " выполняет " << taskName << ", виды товара:";
    for (ProductType type : types_) {
        start << ' ' << type;
    }
    Logger::log(start.str());

    // по отчету на каждый вид товара; получение отчета в терминале зоны хранения - одно действие
    Reports reports;
    for (ProductType type : types_) {
        reports.push_back(storage_.report(type));
        std::this_thread::sleep_for(loader.actionTime());
    }

    // отчеты уходят менеджеру с future
    promise_.set_value(std::move(reports));
    
    Logger::log(who + " закончил " + taskName + ", отчеты переданы менеджеру");
}

std::string InventoryTask::name() const {
    return "Инвентаризация";
}

bool InventoryTask::needsMoreLoaders() {
    return !taken_;
}

std::future<InventoryTask::Reports> InventoryTask::getFuture() {
    return promise_.get_future();
}

std::string InventoryTask::describe(const StorageZone::ProductReport& report) {
    std::ostringstream line;
    line << report.type 
        << ": принято " << report.stats.receivedInReceivingDock
        << ", на хранении " << report.stats.inStorageZone
        << ", в упаковке " << report.stats.inPackingZone
        << ", отправлено " << report.stats.shipped
        << ", паллет " << report.pallets.size();

    // адресов может быть сотни: в лог первые несколько
    const size_t shown = 3;
    int onPallets = 0;
    for (size_t i = 0; i < report.pallets.size(); ++i) {
        onPallets += report.pallets[i].count;
        if (i < shown) {
            line << (i == 0 ? ": " : ", ") << report.pallets[i].address << " -> " << report.pallets[i].count;
        }
    }
    if (report.pallets.size() > shown) {
        line << ", ...";
    }

    return line.str();
}
