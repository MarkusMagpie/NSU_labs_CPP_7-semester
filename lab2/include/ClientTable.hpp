#pragma once

#include <cstddef>
#include <ComplexTypes.hpp>
#include <Table.hpp>

// Таблица клиентов с фиксированной вместимостью.
class ClientTable {
private:
    Table<Client> clients_;
public:
    static constexpr std::size_t capacity = Table<Client>::capacity;

    bool add(const Client& client);
    Client* findById(unsigned long long id);
    const Client* findById(unsigned long long id) const;
    std::size_t size() const;
};
