#include "xml_parser.hpp"
#include "parse_error.hpp"
#include <cctype>
#include <string_view>
#include <utility>

namespace lab4 {

namespace {
class XmlParser {
private:
    std::string text_; // разбираемый текст
    std::size_t pos_ = 0; // индекс текущего символа в разбираемом тексте
    std::size_t line_ = 1; // текущая строка (для сообщений об ошибках)
    std::size_t column_ = 1; // текущий столбец (для сообщений об ошибках)
public:
    XmlParser(std::string text) : text_(std::move(text)) {}

    // ПРИМИТИВЫ (аналогично JsonParser/TomlParser) ---------------------------------------------------------------------
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

    void skip_whitespace() {
        while (!eof() && (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '\n')) {
            advance();
        }
    }

    // для проверки НЕИЗМЕНЯЕМЫХ частей синтаксиса: "<?xml", "?>", "<root>", "<entry", "</entry>", "</root>"
    void expect_syntax(std::string_view literal) {
        for (char expected : literal) {
            expect(expected);
        }
    }

    // необязательный пролог находится переед корнем корнем: <?xml version="1.0" encoding="UTF-8"?>
    // внутрянка его некоснтантна но мне на нее похуй я иду до "?>"
    void skip_prolog_if_any() {
        skip_whitespace();

        if (eof() || text_[pos_] != '<') return;

        if (pos_ + 1 >= text_.size() || text_[pos_ + 1] != '?') return;
        // я понимаю что рассматриваю точно пролог

        expect_syntax("<?xml");
        // альтернатива: advance(); 2 раза - пропуск < и ?
        while (true) {
            if (eof()) {
                error("что-то с объявлением XML-документа, expected \"?>\"");
            }
            if (text_[pos_] == '?' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '>') {
                expect_syntax("?>");
                break;
            }
            advance();
        }
    }
    // РАЗБОР АТРИБУТОВ "key=..." / "type=..." -------------------------------------------------------------------------
    // [A-Za-z0-9_-]+
    // парсер читает имя атрибута до "="
    std::string parse_attr_name() {
        std::size_t start = pos_;
        // advance до знака "="
        while (!eof() && (std::isalnum(static_cast<unsigned char>(text_[pos_]))
                           || text_[pos_] == '_' || text_[pos_] == '-')) {
            advance();
        }

        if (pos_ == start) {
            error("expected an attribute name");
        }

        return text_.substr(start, pos_ - start);
    }

    // key="..." - читать значение ключа
    std::string parse_attribute_value() {
        expect('"');

        std::string result;
        while (true) {
            if (eof()) error("unterminated attribute value, expected '\"'");
            char c = advance();
            if (c == '"') break;
            result += c;
        }

        return result; // хранит value
    }

    // name '=' "value"
    std::pair<std::string, std::string> parse_attribute() {
        std::string name = parse_attr_name();
        skip_whitespace();
        expect('=');
        skip_whitespace();
        std::string value = parse_attribute_value();

        return {std::move(name), std::move(value)};
    }

    // -----------------------------------------------------------------------------------------------------------------
    // <entry key="a" type="int"> метод вызывается после '>' (открывающий тег entry прочитан)
    // задача метода - прочитать значение до начала закрывающего тега
    std::string parse_entry_text() {
        std::string result;

        while (true) {
            if (eof()) error("тег <entry> незавершенный, ожидался \"</entry>\"");
            if (text_[pos_] == '<') {
                break;
            }
            result += advance();
        }

        return result;
    }

    // на вход полностю вырезанное значение из элемента. его нужно превратить из текста в int
    int parse_full_int(const std::string& text) {
        std::size_t idx = 0;
        // мне 0 годиков с момента как узнал что srd::stoi("123abc") вернет не ошибку а 123. поэтому функция и появилась
        try {
            int result = std::stoi(text, &idx);
            if (idx == text.size()) {
                return result;
            }
        } catch (const std::exception&) {
        }

        error("инвалид int value '" + text + "'");
    }

    double parse_full_double(const std::string& text) {
        std::size_t idx = 0;

        try {
            double result = std::stod(text, &idx);
            if (idx == text.size()) {
                return result;
            }
        } catch (const std::exception&) {
        }

        error("инвалид double value '" + text + "'");
    }

    // имею атрибут type с его значением и данные элемента. "Во что превратить данные учитывая type?"
    Value convert_entry_value(const std::string& type, const std::string& text) {
        if (type == "bool") {
            if (text == "true") return Value(true);
            if (text == "false") return Value(false);
            error("invalid bool value '" + text + "', expected \"true\" or \"false\"");
        }
        if (type == "int") return Value(parse_full_int(text));
        if (type == "double") return Value(parse_full_double(text));

        if (type == "string") return Value(text);

        error("unknown type '" + type + "', expected bool/int/double/string");
    }

    // парсинг эдемента: <entry key="..." type="...">значение</entry>
    void parse_entry_element(Object& target) {
        expect_syntax("<entry");

        std::string key;
        std::string type;
        bool has_key = false;
        bool has_type = false;

        // читаю атрибуты в любом порядке просто идя до встречи '>'
        while (true) {
            skip_whitespace();
            if (!eof() && text_[pos_] == '>') break;

            auto [name, value] = parse_attribute();
            if (name == "key") {
                if (has_key) error("duplicate attribute 'key'");
                key = std::move(value);
                has_key = true;
            } else if (name == "type") {
                if (has_type) error("duplicate attribute 'type'");
                type = std::move(value);
                has_type = true;
            } else {
                error("неизвестный атрибут '" + name + "', знаю только 'key' и 'type'");
            }
        }

        if (!has_key) error("<entry> is missing required attribute 'key'");
        if (!has_type) error("<entry> is missing required attribute 'type'");

        expect('>'); // закрыт тег entry

        std::string text = parse_entry_text(); // прочитал значение до начала закрывающегося тега
        expect_syntax("</entry>");

        target.emplace_back(std::move(key), convert_entry_value(type, text));
    }

    Value parse_value() {
        if (eof()) error("unexpected end of input, expected a value");

        skip_prolog_if_any();
        expect_syntax("<root>");

        Object root;
        skip_whitespace();
        // "</" -> тег "</root>"
        while (!(pos_ + 1 < text_.size() && text_[pos_] == '<' && text_[pos_ + 1] == '/')) {
            if (eof()) error("тег <root> не закрыт");
            parse_entry_element(root);
            skip_whitespace();
        }

        expect_syntax("</root>");

        return Value(std::move(root));
    }

    // -----------------------------------------------------------------------------------------------------------------
    Value parse() {
        skip_whitespace();
        Value result = parse_value();
        skip_whitespace();

        if (!eof()) {
            error("ошибка так как есть символы после парсинга документа");
        }

        return result;
    }
};
}  // namespace



// точка входа
Value parse_xml(std::string text) {
    XmlParser parser(text);

    return parser.parse();
}

}  // namespace lab4