#pragma once

#include <chrono>
#include <ostream>

/*
Все величины, заданные в интервалах (от … до …), должны быть сгенерированы
программой автоматически, случайным образом при её старте, или в ходе работы, 
если эти данные о динамическом событии.
*/

namespace limits {
    // Время каждого действия ... от 1 до 5 секунд
    inline int kMinActionTime = 1, kMaxActionTime = 5;

    // ЗОНА ПРИЕМКИ: от 4 до 40 слотов
    inline int kMinReceivingSlots = 4, kMaxReceivingSlots = 40;
    // Если товар не удалось переместить за 5 секунд
    inline int kTimeoutUnits = 5;

    // ЗОНА ХРАНЕНИЯ: от 10 до 500 стеллажей
    inline int kMinRacks = 10, kMaxRacks = 500;
    // Каждый стеллаж содержит от 1 до 6 полок
    inline int kMinShelvesPerRack = 1, kMaxShelvesPerRack = 6;
    // Каждая полка содержит 10 мест под паллеты
    // constexpr используется как размер std::array, должен быть известен при компиляции
    inline constexpr int kPalletPlacesPerShelf = 10;
    // Данная зона (хранения) имеет от 1 до 10 терминалов
    inline int kMinStorageTerminals = 1, kMaxStorageTerminals = 10;

    // ЗОНА УПАКОВКИ: склад может иметь от 2 до 20 столов комплектации
    inline int kMinPackingTables = 2, kMaxPackingTables = 20;
    // Каждый стол имеет от 1 до 3 слотов под паллеты для разбора ...
    inline int kMinTableSortSlots = 1, kMaxTableSortSlots = 3;
    // ... и 2 слота под паллеты для упаковки.
    inline int kTablePackSlots = 2;
    
    // ЗОНА ОТПРАВКИ: от 4 до 40 слотов
    inline int kMinShippingSlots = 4, kMaxShippingSlots = 40;
    // После 5-ой неудачной попытки отправить паллету
    inline int kMaxShippingAttempts = 5;



    // ЗАКАЗЫ: всего городов от 3 до 5
    inline int kMinCities = 3, kMaxCities = 5;
    // ---количество единиц каждого вида товара в заказе: в ТЗ нет
    inline int kMinOrderItems = 1, kMaxOrderItems = 20;

    // ГРУЗЧИКИ: может работать от 2 до 250 грузчиков
    inline int kMinLoaders = 2, kMaxLoaders = 250;

    // МЕНЕДЖЕРЫ: может работать от 1 до 20 менеджеров
    inline int kMinManagers = 1, kMaxManagers = 20;

    // ЗАДАЧА НА РАЗГРУЗКУ: В машине содержаться от 10 до 100 паллет
    inline int kMinTruckPallets = 10, kMaxTruckPallets = 100;
    // Каждая паллета может вмещать от 1 до 100 единиц товара
    inline int kMinPalletItems = 1, kMaxPalletItems = 100;
}

// параметры склада сгенерированные случайно при старте программы
struct Config {
    bool fastMode = false;

    int receivingSlots = 0;
    int shippingSlots = 0;
    int storageRacks = 0;
    int storageTerminals = 0;
    int packingTables = 0;
    int cities = 0;
    int loaders = 0;
    int managers = 0;

    static Config generate(bool fastMode);

    // одна единица времени: 1 секунда в обычном режиме, 1 миллисекунда в быстром
    std::chrono::milliseconds timeUnit();
};

std::ostream& operator<<(std::ostream& os, const Config& config);
