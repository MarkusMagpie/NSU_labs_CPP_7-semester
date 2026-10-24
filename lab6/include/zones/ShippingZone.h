#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <stop_token>
#include <vector>

// ЗОНА ОТПРАВКИ: слоты (от 4 до 40) в которые встают машины для отправки заказов
// машина стоит в слоте пока ее грузят; уезжает когда все места заняты
class ShippingZone {
public:
    // машина для отправки заказов
    struct ShippingTruck {
        int id = 0;       // номер машины
        int city = 0;     // в какой город едет
        int capacity = 0; // сколько мест под паллеты (от 10 до 100)
        int loaded = 0;   // сколько паллет уже погружено
    };

    ShippingZone(int slotCount);

    int slotCount() const;

    /**
     * @brief машина приехала: ставит ее в свободный слот; если все заняты  ждет пока какой-то освободится
     *
     * @param truck приехавшая машина
     * @param stopToken токен остановки ждущего потока
     * @param timeout сколько ждать не больше
     * @return номер слота или -1, если слот так и не освободился (или попросили остановиться)
     */
    int arrive(const ShippingTruck& truck, std::stop_token stopToken, std::chrono::milliseconds timeout);

    // машина уехала: освободить ее слот и разбудить одного ждущего менеджера
    void depart(int slot);

    // сколько слотов сейчас занято
    int busyCount();
private:
    // slots_[i] - машина в слоте i или std::nullopt если слот свободен
    std::vector<std::optional<ShippingTruck>> slots_;
    std::mutex mutex_; // защищает slots_
    std::condition_variable_any slotFreed_; // на ней менеджеры ждут свободный слот, когда все заняты
};
