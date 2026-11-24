#pragma once

#include <cstddef>
#include <memory>

#include <ComplexTypes.hpp>

// таблица вкладов: массив фиксированной вместимости и число занятых элементов
class DepositTable {
private:
    std::unique_ptr<Deposit[]> deposits_;
    std::size_t size_ = 0;

public:
    static constexpr std::size_t capacity = 1e5;

    DepositTable();

    bool add(const Deposit& deposit);
    Deposit* findById(unsigned long long id);
    const Deposit* findById(unsigned long long id) const;
    std::size_t size() const;
};
