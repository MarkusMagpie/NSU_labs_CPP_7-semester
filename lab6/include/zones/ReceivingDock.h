#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <vector>

// ЗОНА ПРИЕМКИ: слоты (от 4 до 40), в которые встают машины на разгрузку
// каждая машина на время разгрузки занимает 1 слот
class ReceivingDock {
private:
    std::vector<bool> busy_; // busy_[i] - занят ли слот i
    std::mutex mutex_; // защищает busy_
    
    std::condition_variable_any slotFreed_; // на ней ждут слот, когда все заняты; releaseSlot будит
public:
    explicit ReceivingDock(int slotCount);

    int slotCount() const;

    /**
     * @brief занимает свободный слот; если все заняты - ждет, пока какой-то слот освободится
     *
     * @param stopToken токен остановки ждущего потока (проснуться сразу при остановке)
     * @param timeout сколько ждать не больше
     * @return номер занятого слота или -1, если слот так и не освободился (или попросили остановиться)
     */
    int occupySlot(std::stop_token stopToken, std::chrono::milliseconds timeout);

    // освободить слот после разгрузки машины и разбудить одного ждущего
    void releaseSlot(int slot);

    // сколько слотов сейчас занято (для лога)
    int busyCount();
};
