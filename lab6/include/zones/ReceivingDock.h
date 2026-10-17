#pragma once

// ЗОНА ПРИЕМКИ
class ReceivingDock {
private:
    int slotCount_; // всего слотов, в которые встают машины на разгрузку (от 4 до 40)
public:
    ReceivingDock(int slotCount);
    int slotCount() const;
};
