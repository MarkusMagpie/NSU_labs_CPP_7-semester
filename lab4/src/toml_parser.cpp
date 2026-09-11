#include "toml_parser.hpp"
#include "parse_error.hpp"
#include <cctype>
#include <string_view>

namespace lab4 {
namespace {
class TomlParser {
private:
    std::string_view text_; // разбираемый текст
    std::size_t pos_ = 0; // индекс текущего символа в разбираемом тексте
    std::size_t line_ = 1; // текущая строка (для сообщений об ошибках)
    std::size_t column_ = 1; // текущий столбец (для сообщений об ошибках)
public:
    TomlParser(std::string_view text) : text_(text) {}

    // ПРИМИТИВЫ (аналогично JsonParser) -------------------------------------------------------------------------------
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

    // если в текущей позиции не expected, кинуть ошибку. иначе advance
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

    // в отличие от JSON перенос строки значим (отделяет одну запись "key = value" от следующей),
    // поэтому пробелы внутри строки и переносы строк пропускаются отдельными функциями
    void skip_inline_whitespace() {
        while (!eof() && (text_[pos_] == ' ' || text_[pos_] == '\t')) {
            advance();
        }
    }

    // если сейчас начинается комментарий '#' - съесть его до конца строки (сам '\n' не трогаем)
    void skip_comment_if_any() {
        if (!eof() && text_[pos_] == '#') {
            while (!eof() && text_[pos_] != '\n') {
                advance();
            }
        }
    }

    // пропускает пустые строки и строки-комментарии между записями "key = value"
    void skip_blank_or_comment_lines() {
        while (true) {
            skip_inline_whitespace();
            skip_comment_if_any();
            if (!eof() && text_[pos_] == '\n') {
                advance(); // съесть перевод строки и попробовать снова (следующая строка тоже может быть пустой)
            } else {
                break;
            }
        }
    }

    // -----------------------------------------------------------------------------------------------------------------
    // документ = (skip_blank_or_comment_lines key_value end_of_line)*
    Value parse() {
        Object root;

        skip_blank_or_comment_lines();
        while (!eof()) {
            parse_key_value(root); // разбор одной записи key=value и вставить в root
            expect_end_of_line(); // expect('\n'); -> между парами нужно иметь 1 символ \n или при raw-string реально перейти на след. строку
            skip_blank_or_comment_lines();
        }

        return Value(std::move(root));
    }

    // после значения ожидается конец строки: необязательные пробелы, необязательный комментарий,
    // а затем '\n' либо конец файла. Если после значения идет что-то еще - ошибка
    // (например "a = 1 2" - два значения на одной строке)
    void expect_end_of_line() {
        skip_inline_whitespace();
        skip_comment_if_any();

        if (!eof()) {
            expect('\n');
        }
    }

    // key '=' value
    // key, пробелы, '=', пробелы, value
    void parse_key_value(Object& target) {
        std::string key = parse_key();

        skip_inline_whitespace();
        expect('=');
        skip_inline_whitespace();

        Value value = parse_value();
        target.emplace_back(std::move(key), std::move(value));
    }

    // [A-Za-z0-9_-]+ либо "..." (использует ту же строку, что и string-значения)
    std::string parse_key() {
        if (eof()) error("unexpected end of input, expected a key");

        // quoted key - в названии ключа требуются пробелы, точки, спецсимволы, ...
        if (peek() == '"') {
            return parse_string_raw();
        }

        // bare key
        std::size_t start = pos_; // ключ начинается здесь
        // while жует символы один за другим пока каждый следующий: буква/цифра (isalnum) или _/-. Иначе - стоп
        while (!eof() && (std::isalnum(static_cast<unsigned char>(text_[pos_]))
                           || text_[pos_] == '_' || text_[pos_] == '-')) {
            advance(); // ++pos
        }

        // цикл выше не съел ни одного символа
        if (pos_ == start) {
            error("ожидался ключ. Таковой не нашелся"); // "= 1"
        }

        return std::string(text_.substr(start, pos_ - start));
    }

    // -----------------------------------------------------------------------------------------------------------------
    // не двигая курсор смотрит на один символ и по нему решает что вызывать дальше
    Value parse_value() {
        if (eof()) error("unexpected end of input, expected a value");

        char c = peek();
        if (c == '"') return Value(parse_string_raw());
        if (c == 't') return parse_keyword("true", Value(true));
        if (c == 'f') return parse_keyword("false", Value(false));
        if (c == '-' || c == '+' || std::isdigit(static_cast<unsigned char>(c))) {
            return parse_number();
        }

        error(std::string("unexpected character '") + c + "', expected bool/int/double/string");
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
    // [+-]?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?
    // необязательный знак, затем обязательно хотя бы одна цифра (это целая часть),
    // затем необязательно . + цифры (дробная часть), затем необязательно e/E + необязательный знак + цифры (экспонента)
    // Отличие от JSON: разрешен ведущий '+'
    Value parse_number() {
        std::size_t start = pos_;
        bool is_double = false;

        // 1) съедание ведущего знака при наличии
        if (!eof() && (peek() == '-' || peek() == '+')) advance();

        // 2) обязательная целая часть
        if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
            error("invalid number: целая часть обязательна"); // "a = -"
        }
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }

        // 3) опциональная дробная часть
        if (!eof() && peek() == '.') {
            is_double = true;
            advance(); // скип точки
            if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                error("invalid number: expected digits after decimal point");
            }
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        // 4) экспонента
        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            is_double = true;
            advance(); // скип e или E
            if (!eof() && (peek() == '+' || peek() == '-')) advance(); // необязательный знак экспоненты
            if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                error("invalid number: после знака экспоненты должно идти число");
            }
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        // 5) конвертация
        // до этого только двигался вправо
        std::string res(text_.substr(start, pos_ - start));
        try {
            if (is_double) {
                return Value(std::stod(res)); // . или e/E -> StringToDouble
            }

            return Value(std::stoi(res)); // StringToInt
        } catch (const std::out_of_range&) {
            error("number literal is out of range: " + res);
        }
    }

    // "..." с escape-последовательностями (идентично JSON)
    std::string parse_string_raw() {
        expect('"'); // съесть открывающую кавычку

        std::string result;
        while (true) {
            if (eof()) error("unterminated string literal");
            char c = advance(); // берется текущий символ и сдвигается
            if (c == '"') break;
            if (c == '\\') {
                // символы \ + n в исходнике должны превратиться в один другой символ на выходе
                result += parse_escape_seq(); // += '\n'
            } else {
                result += c;
            }
        }

        return result;
    }

    // "line1\nline2": result="line1"; "\" -> parse_escape_seq(): 'n' -> "\n" -> result="line1\n"; result="line1\nline2"
    std::string parse_escape_seq() {
        if (eof()) error("unterminated escape sequence");

        char c = advance(); // чтение символа следующего после backlash
        // возврат 1 символа
        switch (c) {
            case '"': return "\"";
            case '\\': return "\\";
            case 'b': return "\b";
            case 'f': return "\f";
            case 'n': return "\n";
            case 'r': return "\r";
            case 't': return "\t";
            default: error(std::string("invalid escape sequence '\\") + c + "'");
        }
    }
};
}  // namespace

// точка входа
Value parse_toml(std::string_view text) {
    TomlParser parser(text);

    return parser.parse();
}
}  // namespace lab4