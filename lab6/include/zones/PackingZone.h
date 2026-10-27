#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <vector>

// ЗОНА УПАКОВКИ
// 1 грузчик бронирует 1 стол на время комплектации заказа
class PackingZone {
private:
    struct Table {
        int sortSlots = 0;
        int packSlots = 0;
        bool booked = false;
    };
    std::vector<Table> tables_;
    std::mutex mutex_; // защита booked у столов
    
    std::condition_variable_any tableFreed_; // на ней грузчики ждут свободный стол; releaseTable будит
public:
    explicit PackingZone(int tableCount);

    int tableCount() const;

    /**
     * @brief бронь свободного стола для комплектации грузчиком; если все заняты - ждет, пока какой-то освободится
     *
     * @param stopToken токен остановки потока грузчика
     * @param timeout сколько ждать не больше (по ТЗ: 5 единиц времени)
     * @return номер стола или -1 если стол так и не освободился (или попросили остановиться)
     */
    int bookTable(std::stop_token stopToken, std::chrono::milliseconds timeout);

    // снять бронь со стола и разбудить одного грузчика ждущего стола
    void releaseTable(int table);

    // сколько слотов для разбора у стола
    int sortSlots(int table) const;
    // сколько слотов для упаковки у стола
    int packSlots(int table) const;

    // сколько столов сейчас забронировано
    int busyCount();
};
