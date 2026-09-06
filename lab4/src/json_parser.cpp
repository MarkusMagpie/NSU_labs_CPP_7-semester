#include "json_parser.hpp"

#include <cctype>
#include <string_view>

#include "parse_error.hpp"

namespace lab4 {
namespace {
// умеет разбирать все значения JSON: скаляры (true/false, числа, строки),
// составные значения (массивы, объекты), которые рекурсивно состоят
// из тех же значений
class JsonParser {
private:
    std::string_view text_; // разбираемый текст
    std::size_t pos_ = 0; // индекс текущего символа в разбираемом тексте
    std::size_t line_ = 1; // текущая строка (для сообщений об ошибках)
    std::size_t column_ = 1; // текущий столбец (для сообщений об ошибках)
public:
    JsonParser(std::string_view text) : text_(text) {}

    // ПРИМИТИВЫ -------------------------------------------------------------------------------------------------------
    bool eof() const {
        return pos_ >= text_.size();
    }

    // возвращает текущий символ
    char peek() const {
        if (eof()) {
            error("unexpected end of input");
        }

        return text_[pos_];
    }

    // возвращает текущий символ и сдвигает pos_ на 1 дальше обновляя line_/column_ для будущих сообщений об ошибках
    char advance() {
        char c = peek();
        ++pos_;
        if (c == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }

        return c;
    }

    // ели в текущей позиции не expected, кинуть ошибку. иначе advance
    void expect(char expected) {
        if (eof() || text_[pos_] != expected) {
            error(std::string("expected '") + expected + "'");
        }

        advance();
    }

    // бросить ParseError с текущими line_/column_
    void error(const std::string& message) const {
        throw ParseError(message, line_, column_);
    }

    void skip_whitespace() {
        while (!eof()) {
            char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                advance();
            } else {
                break;
            }
        }
    }

    // -----------------------------------------------------------------------------------------------------------------
    // скип пробелов, разобрать json, скип пробелов в конце, если после этого в тексте чтото осталось кидаю плотную ошибку
    // например "42 43" - не один JSON документ -> ошибка
    Value parse() {
        skip_whitespace();
        Value result = parse_value();
        skip_whitespace();

        if (!eof()) {
            error("ошибка так как есть символы после парсинга документа");
        }

        return result;
    }

    // -----------------------------------------------------------------------------------------------------------------
    Value parse_value() {
        // не двигая курсор смотрит на один символ и по нему решает что вызывать дальше
        if (eof()) error("unexpected end of input, expected a value");

        char c = peek();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == 't') return parse_keyword("true", Value(true));
        if (c == 'f') return parse_keyword("false", Value(false));
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return parse_number();
        }
        if (c == '"') return Value(parse_string_raw());

        error(std::string("unexpected character '") + c + "'");
    }

    // '[' (value (',' value)*)? ']'
    Value parse_array() {
        expect('[');
        Array items;

        skip_whitespace();

        if (!eof() && text_[pos_] == ']') {
            advance(); // съесть ']'
            return Value(items);
        }

        while (true) {
            skip_whitespace();
            items.push_back(parse_value()); // рекурсия
            skip_whitespace();

            if (!eof() && peek() == ',') {
                advance();
                continue;  // после запятой снова жду значение. ОТДЕЛЬНЫЙ КЕЙС: "[1,]" упадет!
            }
            break;
        }

        skip_whitespace();
        expect(']');

        return Value(std::move(items));
    }

    // '{' (string ':' value (',' string ':' value)*)? '}'
    Value parse_object() {
        expect('{');
        Object members;

        skip_whitespace();
        if (!eof() && peek() == '}') {
            advance();
            return Value(members);
        }

        while (true) {
            skip_whitespace();
            if (eof() || text_[pos_] != '"') {
                error("ожидалась что ключ имеет тип данных string");
            }
            std::string key = parse_string_raw();

            skip_whitespace();
            expect(':'); // сепаратор ключа и значения
            skip_whitespace();

            Value value = parse_value();
            members.emplace_back(std::move(key), std::move(value));

            skip_whitespace();
            if (!eof() && peek() == ',') {
                advance();
                continue;
            }
            break;
        }

        skip_whitespace();
        expect('}');

        return Value(std::move(members));
    }

    // дальше в тексте идет "true"/"false"?
    // если да то возвращает заранее подготовленное значение
    Value parse_keyword(std::string_view keyword, Value result) {
        // t->r->u->e и f->a->l->s->e
        for (char expected : keyword) {
            expect(expected); // text_[0] == 't'? -> advance ...
        }

        return result;
    }

    // -----------------------------------------------------------------------------------------------------------------
    // -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?
    // пример: -42.5e+3
    // встретил '.' или экспоненту -> double, иначе int
    Value parse_number() {
        int start = pos_;
        bool is_double = false;

        if (!eof() && peek() == '-') advance(); // съедание минуса при наличии

        if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
            error("invalid number: expected a digit");
        }
        if (peek() == '0') {
            advance();  // ведущий ноль в JSON не может ничем продолжаться
        } else {
            // цикл while съедает цифры одну за другой пока они реально цифры
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
                advance();
        }

        if (!eof() && peek() == '.') {
            is_double = true;
            advance();
            if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                error("invalid number: expected digits after decimal point");
            }
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
        }

        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            is_double = true;
            advance(); // скип e или E
            if (!eof() && (peek() == '+' || peek() == '-')) advance(); // необязательный знак экспоненты
            if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                error("invalid number: после знака экспоненты должно идти число");
            }
            // поедание цифр в экмпоненте
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        std::string literal(text_.substr(start, pos_ - start));
        try {
            if (is_double) return Value(std::stod(literal)); // . или e/E -> StringToDouble: std::stod("-42.5e+3") -> -42500.0
            return Value(std::stoi(literal));  // StringToInt
        } catch (const std::out_of_range&) {
            error("number literal is out of range: " + literal);
        }
    }

    std::string parse_string_raw() {
        expect('"'); // съесть открывающую кавычку

        std::string result;
        while (true) {
            if (eof()) error("unterminated string literal");
            char c = advance(); // берется текущий символ и сдвигается
            if (c == '"') break;
            if (c == '\\') {
                result += parse_escape_seq();
            } else {
                result += c;
            }
        }

        return result;
    }

    std::string parse_escape_seq() {
        if (eof()) error("unterminated escape sequence");

        char c = advance();
        switch (c) {
            case '"': return "\"";
            // https://ru.cppreference.com/c/language/escape
            case '\\': return "\\";
            case 'b': return "\b";
            case 'f': return "\f";
            case 'n': return "\n";
            case 'r': return "\r";
            case 't': return "\t";
            default: error(std::string("invalid escape sequence '\\") + c + "'");
        }
    }

    // "line1\nline2": result="line1"; "\" -> parse_escape_seq(): 'n' -> "\n" -> result="line1\n"; result="line1\nline2"
};
}  // namespace



// точка входа
Value parse_json(std::string_view text) {
    JsonParser parser(text);

    return parser.parse();
}
}  // namespace lab4