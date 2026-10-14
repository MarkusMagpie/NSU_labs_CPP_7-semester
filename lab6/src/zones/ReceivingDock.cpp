#include <iostream>

#include <zones/ReceivingDock.h>

ReceivingDock::ReceivingDock(int slotCount) : slotCount_(slotCount) {
    std::cout << "Зона приемки создана: " << slotCount_ << " слотов\n";
}

int ReceivingDock::slotCount() const {
    return slotCount_;
}
