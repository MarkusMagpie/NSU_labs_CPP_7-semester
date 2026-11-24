#include <DepositTable.hpp>

DepositTable::DepositTable()
    : deposits_(std::make_unique<Deposit[]>(capacity))
{
}

bool DepositTable::add(const Deposit& deposit) {
    if (size_ == capacity) {
        return false;
    }

    deposits_[size_] = deposit;
    ++size_;
    
    return true;
}

Deposit* DepositTable::findById(unsigned long long id) {
    for (std::size_t i = 0; i < size_; ++i) {
        if (deposits_[i].id == id) {
            return &deposits_[i];
        }
    }

    return nullptr;
}

const Deposit* DepositTable::findById(unsigned long long id) const {
    for (std::size_t i = 0; i < size_; ++i) {
        if (deposits_[i].id == id) {
            return &deposits_[i];
        }
    }

    return nullptr;
}

std::size_t DepositTable::size() const {
    return size_;
}
