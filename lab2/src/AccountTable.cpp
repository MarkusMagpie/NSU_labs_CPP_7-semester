#include <AccountTable.hpp>

bool AccountTable::add(const Account& new_account) {
    return accounts_.add(new_account);
}

Account* AccountTable::findById(unsigned long long id) {
    for (std::size_t i = 0; i < accounts_.size(); ++i) {
        Account* account = accounts_.get(i);
        if (account->id == id) {
            return account;
        }
    }

    return nullptr;
}

const Account* AccountTable::findById(unsigned long long id) const {
    for (std::size_t i = 0; i < accounts_.size(); ++i) {
        const Account* account = accounts_.get(i);
        if (account->id == id) {
            return account;
        }
    }
    
    return nullptr;
}

std::size_t AccountTable::size() const {
    return accounts_.size();
}
