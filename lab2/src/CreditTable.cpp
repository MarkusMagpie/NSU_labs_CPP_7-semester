#include <CreditTable.hpp>

bool CreditTable::add(const Credit& credit) {
    return credits_.add(credit);
}

Credit* CreditTable::findById(unsigned long long id) {
    for (std::size_t i = 0; i < credits_.size(); ++i) {
        Credit* credit = credits_.get(i);
        if (credit->id == id) {
            return credit;
        }
    }

    return nullptr;
}

const Credit* CreditTable::findById(unsigned long long id) const {
    for (std::size_t i = 0; i < credits_.size(); ++i) {
        const Credit* credit = credits_.get(i);
        if (credit->id == id) {
            return credit;
        }
    }
    
    return nullptr;
}

std::size_t CreditTable::size() const {
    return credits_.size();
}
