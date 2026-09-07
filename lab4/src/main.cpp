#include <iostream>
#include <stdexcept>
#include "format.hpp"
#include "io_utils.hpp"
#include "json_parser.hpp"
#include "json_writer.hpp"
#include "parse_error.hpp"
#include "toml_parser.hpp"
#include "toml_writer.hpp"

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

    // XML пока не реализован ни на чтение, ни на запись
    if (*input_format == lab4::Format::Xml || *output_format == lab4::Format::Xml) {
        std::cerr << "Conversion between different formats ("
            << lab4::to_string(*input_format) << " -> "
            << lab4::to_string(*output_format) << ") is not implemented yet\n";

        return 1;
    }

    const std::string input = lab4::read_all(std::cin);

    try {
        // input_format -> Value (дерево одно для любого входного формата)
        const lab4::Value tree = (*input_format == lab4::Format::Json)
            ? lab4::parse_json(input)
            : lab4::parse_toml(input);

        // Value -> output_format
        if (*output_format == lab4::Format::Json) {
            lab4::write_json(tree, std::cout);
        } else {
            lab4::write_toml(tree, std::cout);
        }
    } catch (const lab4::ParseError& e) {
        // ошибка синтаксиса во входном документе (JSON/TOML)
        std::cerr << e.what() << "\n";

        return 1;
    }

    return 0;
}