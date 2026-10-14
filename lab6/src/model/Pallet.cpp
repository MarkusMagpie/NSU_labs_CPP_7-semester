#include <model/Pallet.h>

std::ostream& operator<<(std::ostream& os, const Pallet& pallet) {
    return os << pallet.type << " x" << pallet.count;
}
