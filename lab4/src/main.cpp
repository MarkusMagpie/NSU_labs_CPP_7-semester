#include <iostream>
#include "format.hpp"
#include "io_utils.hpp"
#include "json_parser.hpp"
#include "json_writer.hpp"
#include "parse_error.hpp"

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
    std::optional<lab4::Value> tree;

    if (*input_format == lab4::Format::Json && *output_format == lab4::Format::Json) {
        try {
            tree = lab4::parse_json(input); // JSON -> Value
            lab4::write_json(*tree, std::cout); // Value -> JSON
        } catch (const lab4::ParseError& e) {
            std::cerr << e.what() << "\n";

            return 1;
        }

        return 0;
    }

    std::cerr << "Conversion between different formats ("
        << lab4::to_string(*input_format) << " -> "
        << lab4::to_string(*output_format) << ") is not implemented yet\n";
    return 1;
}
