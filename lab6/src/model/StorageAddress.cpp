#include <model/StorageAddress.h>

std::ostream& operator<<(std::ostream& os, const StorageAddress& address) {
    return os << "[стеллаж " << address.rack 
        << ", полка " << address.shelf 
        << ", место " << address.place << ']';
}
