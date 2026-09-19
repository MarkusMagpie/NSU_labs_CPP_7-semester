#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include "format.hpp"
#include "io_utils.hpp"
#include "json_parser.hpp"
#include "json_writer.hpp"
#include "parse_error.hpp"
#include "toml_parser.hpp"
#include "toml_writer.hpp"
#include "value.hpp"
#include "xml_parser.hpp"
#include "xml_writer.hpp"

namespace {
    std::string write_json_to_string(const lab4::Value& value) {
        std::ostringstream out;
        lab4::write_json(value, out);

        return out.str();
    }

    // json_text парсится JSON парсером и сериализуется TOML writerом
    std::string json_to_toml(const std::string& json_text) {
        std::ostringstream out;
        lab4::write_toml(lab4::parse_json(json_text), out);

        return out.str();
    }

    // json_text парсится (десериализация) JSON парсером; сериализация XML writerом
    std::string json_to_xml(const std::string& json_text) {
        std::ostringstream out; // поток вывода для записи в строки
        lab4::write_xml(lab4::parse_json(json_text), out);

        return out.str();
    }

    // json_text парсится JSON парсером, пишется в формате TOML, парсится обратно TOML парсером и снова пишется JSON writerом
    std::string json_via_toml(std::string& json_text) {
        std::ostringstream toml_out;
        lab4::write_toml(lab4::parse_json(json_text), toml_out); // JSON -> toml

        return write_json_to_string(lab4::parse_toml(toml_out.str())); // toml -> json
    }


    // из условия: JSON => TOML; TOML => XML; XML => JSON и сравнить изначальные данные с данными, полученными в конце
    // json текст парсится json парсером, пишется в формате томл, томл текст парсится томл парсером, пишется в формате xml,
    // xml текст парсится xml прасером и полученный объект Value пишется в json формате json writerом.
    std::string json_via_toml_via_xml(const std::string& json_text) {
        std::ostringstream toml_out; // поток вывода для записи в строки
        lab4::write_toml(lab4::parse_json(json_text), toml_out);

        std::ostringstream xml_out;
        lab4::write_xml(lab4::parse_toml(toml_out.str()), xml_out);

        return write_json_to_string(lab4::parse_xml(xml_out.str()));
    }
}// namespace