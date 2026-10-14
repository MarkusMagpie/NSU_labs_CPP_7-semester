#include <iostream>

#include <zones/ShippingZone.h>

ShippingZone::ShippingZone(int slotCount) : slotCount_(slotCount) {
    std::cout << "Зона отправки создана: " << slotCount_ << " слотов\n";
}

int ShippingZone::slotCount() const {
    return slotCount_;
}
