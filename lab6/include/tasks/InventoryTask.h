#pragma once

#include <atomic>
#include <future>
#include <string>
#include <vector>

#include <model/ProductType.h>
#include <tasks/Task.h>
#include <zones/StorageZone.h>

// ЗАДАЧА НА ИНВЕНТАРИЗАЦИЮ: получить отчеты по указанным видам товара из зоны хранения
// ! получить задачу могут несколько грузчиков, выполнить должен только один
class InventoryTask : public Task {
public:
    // результат задачи: по 1 отчету на каждый вид товара (их 3)
    using Reports = std::vector<StorageZone::ProductReport>;
    std::promise<Reports> promise_;

    InventoryTask(int id, const std::vector<ProductType>& types, StorageZone& storage);

    void execute(Loader& loader) override;
    std::string name() const override;

    // пока задачу никто не взял в работу, ее могут получать другие грузчики
    bool needsMoreLoaders() override;

    // future через который придут отчеты
    std::future<Reports> getFuture();

    // текст одного отчета для вывода в лог
    static std::string describe(const StorageZone::ProductReport& report);
private:
    int id_; // номер задачи (для лога: какой отчет к какой задаче относится)
    std::vector<ProductType> types_; // по каким видам товара нужен отчет
    StorageZone& storage_; // зона хранения: в ее терминале строятся отчеты

    std::atomic<bool> taken_{false}; // взял ли уже кто-то задачу в работу

    // "обещание" отчетов: исполняет грузчик (set_value), получает менеджер через future
};
