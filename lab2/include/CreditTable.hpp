#pragma once

#include <cstddef>
#include <ComplexTypes.hpp>
#include <Table.hpp>

// таблица кредитов: массив фиксированной вместимости (capacity) и число занятых элементов
class CreditTable {
private:
    Table<Credit> credits_;
public:
    static constexpr std::size_t capacity = Table<Credit>::capacity;

    bool add(const Credit& credit);
    Credit* findById(unsigned long long id);
    const Credit* findById(unsigned long long id) const;
    std::size_t size() const;
};
