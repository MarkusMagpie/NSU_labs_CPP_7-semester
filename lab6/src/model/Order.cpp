#include <model/Order.h>
#include <utils/Config.h>
#include <utils/RandomGenerator.h>

using namespace limits;

Order Order::generate(int id, int cities, std::mt19937& gen) {
    Order order;
    order.id = id;
    order.city = RandomGenerator::getRandom(gen, 0, cities - 1);
    for (ProductType tovar_type : kAllProductTypes) {
        order.items[tovar_type] = RandomGenerator::getRandom(gen, kMinOrderItems, kMaxOrderItems);
    }

    return order;
}

std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << "заказ #" << order.id << " в город " << order.city << ':';
    
    for (const auto& [type, count] : order.items) {
        os << ' ' << type << " x" << count << ';';
    }
    return os << '\n';
}
