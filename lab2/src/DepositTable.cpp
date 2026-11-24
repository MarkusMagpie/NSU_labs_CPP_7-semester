#include <DepositTable.hpp>

bool DepositTable::add(const Deposit& new_deposit) {
    return deposits_.add(new_deposit);
}

Deposit* DepositTable::findById(unsigned long long id) {
    for (std::size_t i = 0; i < deposits_.size(); ++i) {
        Deposit* deposit = deposits_.get(i);
        if (deposit->id == id) {
            return deposit;
        }
    }

    return nullptr;
}

const Deposit* DepositTable::findById(unsigned long long id) const {
    for (std::size_t i = 0; i < deposits_.size(); ++i) {
        const Deposit* deposit = deposits_.get(i);
        if (deposit->id == id) {
            return deposit;
        }
    }

    return nullptr;
}

std::size_t DepositTable::size() const {
    return deposits_.size();
}
