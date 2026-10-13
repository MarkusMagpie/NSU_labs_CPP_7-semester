#include <iostream>
#include <string_view>

#include <utils/Config.h>

int main(int argc, char* argv[]) {
    bool fast_mod = argc > 1 && std::string_view(argv[1]) == "--fast";

    Config config = Config::generate(fast_mod);
    std::cout << config;

    return 0;
}
