#include <AccountTable.hpp>

AccountTable::AccountTable()
    : accounts_(std::make_unique<Account[]>(capacity))
{
}

bool AccountTable::add(const Account& account) {
    if (size_ == capacity) {
        return false;
    }

    accounts_[size_] = account;
    ++size_;

    return true;
}

Account* AccountTable::findById(unsigned long long id) {
    for (std::size_t i = 0; i < size_; ++i) {
        if (accounts_[i].id == id) {
            return &accounts_[i];
        }
    }

    return nullptr;
}

const Account* AccountTable::findById(unsigned long long id) const {
    for (std::size_t i = 0; i < size_; ++i) {
        if (accounts_[i].id == id) {
            return &accounts_[i];
        }
    }
    
    return nullptr;
}

std::size_t AccountTable::size() const {
    return size_;
}
