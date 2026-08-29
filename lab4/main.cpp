#include <iostream>
#include "format.hpp"
#include "io_utils.hpp"

int main(int argc, char** argv) {
    if (argc != 3) return 1;

    const auto input_format = lab4::parse_format(argv[1]);
    const auto output_format = lab4::parse_format(argv[2]);

    if (!input_format) {
        std::cerr << "Unknown input format was used: " << argv[1] << "\n";
        return 1;
    }
    if (!output_format) {
        std::cerr << "Unknown output format was used: " << argv[2] << "\n";
        return 1;
    }

    const std::string input = lab4::read_all(std::cin);

    if (*input_format == *output_format) {
        std::cout << input;

        return 0;
    } else {
        std::cerr << "Conversion between different formats ("
        << lab4::to_string(*input_format) << " -> "
        << lab4::to_string(*output_format) << ") is not implemented yet\n";
    }

    return 0;
}
