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

    // что грузчик увидел на доске: в каком слоте и какая именно машина
    struct TruckRef {
        int slot = -1;
        int truckId = 0;
    };

    // итог отправки паллеты: удалось ли, за сколько попыток, и уехавшая машина если паллета была последней
    struct ShipResult {
        bool shipped = false;
        int attempts = 0;
        std::optional<ShippingTruck> departedTruck;
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

    // машина в город city со свободным местом (по доске занятости) или std::nullopt
    std::optional<TruckRef> findTruck(int city) const;

    /**
     * @brief ждет, пока на доске появится машина в город city со свободным местом
     *
     * Грузчики спят на boardChanged_; его будят методы arrive()/load()/depart() (через notify_all)
     *
     * @param city город заказа
     * @param stopToken токен остановки потока грузчика (проснется сразу при остановке)
     * @param timeout сколько ждать не больше
     * @return машина или std::nullopt
     */
    std::optional<TruckRef> waitForTruck(int city, 
        std::stop_token stopToken, 
        std::chrono::milliseconds timeout);

    /**
     * @brief отправить паллету заказа в город city - алгоритм грузчика в зоне отправки по ТЗ
     *
     * Попытка: посмотреть доску; 
     * если машины в город нет - ждать ее от 1 до 5 единиц времени;
     * если машина есть - погрузить; 
     * если машина уехала/заполнилась - следующая попытка.
     * Не больше maxAttempts попыток (по ТЗ 5).
     *
     * @param city город заказа
     * @param pallet паллета для отправки
     * @param actionTime время одного действия грузчика (погрузка)
     * @param timeUnit единица времени (1 с или 1 мс) - для ожидания машины 1-5 единиц
     * @param stopToken токен остановки потока грузчика
     */
    ShipResult ship(int city, 
        const Pallet& pallet, 
        std::chrono::milliseconds actionTime,
        std::chrono::milliseconds timeUnit, 
        std::stop_token stopToken);

    Board board() const;
private:
    Board slots_;

    // поиск машины по доске БЕЗ захвата мьютекса - вызывать только под уже захваченным mutex_
    // (findTruck и условие ожидания в waitForTruck). Повторно захватывать mutex_ тем же потоком нельзя
    std::optional<TruckRef> findTruckLocked(int city) const;

    // защищает slots_: читатели - shared_lock, писатели (приезд/отъезд/погрузка) - unique_lock
    mutable std::shared_mutex mutex_;

    // на ней менеджеры ждут свободный слот, когда все заняты
    std::condition_variable_any slotFreed_;

    // на ней ждут машины в свой город грузчики
    std::condition_variable_any boardChanged_;

    // doors_[i] - вход в машину в слоте i: "в машине для погрузки может находиться только один грузчик"
    std::vector<std::mutex> doors_;
};
