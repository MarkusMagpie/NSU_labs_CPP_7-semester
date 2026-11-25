#include <ClientTable.hpp>

bool ClientTable::add(const Client& client) {
    return clients_.add(client);
}

Client* ClientTable::findById(unsigned long long id) {
    for (std::size_t i = 0; i < clients_.size(); ++i) {
        Client* client = clients_.get(i);
        if (client->id == id) {
            return client;
        }
    }

    return nullptr;
}

const Client* ClientTable::findById(unsigned long long id) const {
    for (std::size_t i = 0; i < clients_.size(); ++i) {
        const Client* client = clients_.get(i);
        if (client->id == id) {
            return client;
        }
    }

    return nullptr;
}

std::size_t ClientTable::size() const {
    return clients_.size();
}
