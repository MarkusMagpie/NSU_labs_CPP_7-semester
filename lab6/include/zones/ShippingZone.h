#pragma once

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stop_token>
#include <vector>

#include <model/Pallet.h>
#include <model/ProductType.h>

// ЗОНА ОТПРАВКИ: слоты (от 4 до 40) в которые встают машины для отправки заказов
// машина стоит в слоте пока ее грузят; уезжает когда все места заняты.
// slots_ одновременно служит доской занятости: по ней грузчики ищут машину в свой город
class ShippingZone {
public:
    ShippingZone(int slotCount);

    struct ShippingTruck {
        int id = 0;       // номер машины
        int city = 0;     // в какой город едет
        int capacity = 0; // сколько мест под паллеты (от 10 до 100)
        int loaded = 0;   // сколько паллет уже погружено
        std::map<ProductType, int> cargo;
    };

    // доска занятости: board[i] - машина в слоте i или std::nullopt если слот свободен
    using Board = std::vector<std::optional<ShippingTruck>>;

    // чем закончилась погрузка паллеты в машину
    enum class LoadStatus {
        Loaded,
        Departed,
        TruckGone
    };
    struct LoadResult {
        LoadStatus status = LoadStatus::TruckGone;
        ShippingTruck truck;
    };

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

    // машина уехала: освободить ее слот, разбудить одного ждущего менеджера и оповестить грузчиков
    void depart(int slot);

    /**
     * @brief погрузить паллету в машину; в машине одновременно находится только один грузчик
     *
     * Грузчик встает в очередь у двери машины, внутри заново проверяет, что машина та же по truckId
     * и в ней есть место, грузит и отмечает погрузку. Если заняты все места -> машина уезжает.
     *
     * @param slot слот машины (найден по доске занятости)
     * @param truckId номер машины которую грузчик видел на доске занятости
     * @param pallet паллета для отправки
     * @param actionTime сколько длится погрузка
     */
    LoadResult load(int slot, int truckId, const Pallet& pallet, std::chrono::milliseconds actionTime);

    // сколько слотов сейчас занято
    int busyCount() const;

    // ДОСКА ЗАНЯТОСТИ ----------------------------------------------------------------------------------

    // слот машины в city гдеесть свободное место
    int findTruck(int city) const;

    Board board() const;
private:
    Board slots_;

    // защищает slots_: читатели - shared_lock, писатели (приезд/отъезд/погрузка) - unique_lock
    mutable std::shared_mutex mutex_;

    // на ней менеджеры ждут свободный слот, когда все заняты
    std::condition_variable_any slotFreed_;

    // на ней ждут машины в свой город грузчики
    std::condition_variable_any boardChanged_;

    // doors_[i] - вход в машину в слоте i: "в машине для погрузки может находиться только один грузчик"
    std::vector<std::mutex> doors_;
};
