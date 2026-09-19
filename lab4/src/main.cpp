#include <iostream>
#include <stdexcept>
#include <variant>
#include "format.hpp"
#include "io_utils.hpp"
#include "json_parser.hpp"
#include "json_writer.hpp"
#include "parse_error.hpp"
#include "toml_parser.hpp"
#include "toml_writer.hpp"
#include "xml_parser.hpp"
#include "xml_writer.hpp"

int main(int argc, char** argv) {
    if (argc != 3) return 1;

    auto input_format = lab4::parse_format(argv[1]);
    auto output_format = lab4::parse_format(argv[2]);

    if (!input_format) {
        std::cerr << "Входной формат не входит в следующий список {JSON, TOML, XML}: " << argv[1] << "\n";
        return 1;
    }
    if (!output_format) {
        std::cerr << "ВЫходной формат не входит в список {JSON, TOML, XML}: " << argv[2] << "\n";
        return 1;
    }

    std::string input = lab4::read_all(std::cin);

    try {
        // ДЕСЕРИАЛИЗАЦИЯ: input_format -> Value
        lab4::Value tree = [&]() -> lab4::Value {
            if (*input_format == lab4::Format::Json) {
                return lab4::parse_json(input);
            }
            if (*input_format == lab4::Format::Toml) {
                return lab4::parse_toml(input);
            }
            return lab4::parse_xml(input);
        }(); // сразу лямбда функцию вызвал

        // Преобразование абстрактного дерева в заданный выходной формат представления = СЕРИАЛИЗАЦИЯ
        // Value -> output_format
        if (*output_format == lab4::Format::Json) {
            lab4::write_json(tree, std::cout);
        } else if (*output_format == lab4::Format::Toml) {
            lab4::write_toml(tree, std::cout);
        } else {
            lab4::write_xml(tree, std::cout);
        }
    } catch (const lab4::ParseError& e) {
        // ошибка синтаксиса во входном документе (JSON/TOML/XML)
        std::cerr << "ошибка синтаксиса во входном документе: " << e.what() << "\n";

        return 1;
    } catch (const std::bad_variant_access& e) {
        std::cerr << "внутренняя ошибка работы as<T>() (баг): " << e.what() << "\n";

        return 1;
    } catch (const std::invalid_argument& e) {
        // дерево Value несовместимо с выбранным выходным форматом
        std::cerr << "дерево несовместимо с выходным форматом: " << e.what() << "\n";

        return 1;
    }

    return 0;
}