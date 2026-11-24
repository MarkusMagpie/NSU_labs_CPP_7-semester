#include <cstdio>
#include <cstdlib>
#include <exception>

#include <AccountTable.hpp>
#include <InputReader.hpp>

int main() {
    try {
        AccountTable accounts;
        readAccounts(accounts);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Ошибка чтения: %s\n", error.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
