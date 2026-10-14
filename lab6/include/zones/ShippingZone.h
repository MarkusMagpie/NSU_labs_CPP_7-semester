#pragma once

// ЗОНА ОТПРАВКИ
class ShippingZone {
private:
    int slotCount_; // сколько слотов в которые встают машины для отправки заказов (4 - 40)
public:
    explicit ShippingZone(int slotCount);
    int slotCount() const;
};
