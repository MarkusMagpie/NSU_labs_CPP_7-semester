#include "regex_engine.hpp"

std::string parseSet(const std::string& pattern, int& i) {
    std::string chars;
    while (i < pattern.size() && pattern[i] != ']') {
        char c = pattern[i++];
        // кейс "[a-z]"
        if (i + 1 < pattern.size() && pattern[i] == '-' && pattern[i + 1] != ']') {
            char to = pattern[i + 1];
            i += 2;
            for (char ch = c; ch <= to; ++ch)
                chars += ch;
        } else {
            chars += c;
        }
    }
    if (i < pattern.size()) ++i; // skip ']'

    return chars;
}



Regex::Regex(const std::string& pattern) {
    int i = 0;
    while (i < pattern.size()) {
        std::unique_ptr<Atom> atom;

        if (pattern[i] == '[') {
            ++i; // skip '['
            atom = std::make_unique<GroupAtom>(parseSet(pattern, i));
        } else if (pattern[i] == '.') {
            atom = std::make_unique<AnyAtom>();
            ++i;
        } else {
            atom = std::make_unique<LiteralAtom>(pattern[i]);
            ++i;
        }

        Modifier mod = Modifier::Once;
        if (i < pattern.size()) {
            if (pattern[i] == '*') { 
                mod = Modifier::Star; 
                ++i; 
            } else if (pattern[i] == '+') { 
                mod = Modifier::Plus;     
                ++i; 
            } else if (pattern[i] == '?') { 
                mod = Modifier::Question; 
                ++i; 
            }
        }

        tokens.push_back({std::move(atom), mod});
        // Token лежат в std::vector. Когда вектор очищается (Regex уничтожается), то все Token в нем удаляются,
        // а вместе с ними и unique_ptr внутри каждого токена автоматически вызывает delete на свой Atom
        // это аргумент в пользу unique_ptr, иначе бы пришлось писать деструктор.
        // а от меня в рамках задачи требуют "управление динамической памятью вручную" - unique_ptr автоматизирует эту задачу.
    }
}

// Regex::~Regex() {
//     for (Token& tok : tokens) {
//         delete tok.atom;
//     }
// }

bool Regex::matchFrom(int tokIdx, const std::string& text, int textIdx) {
    if (tokIdx == tokens.size()) {
        return textIdx == text.size();
    }

    Token& tok = tokens[tokIdx];
    Atom& atom = *tok.atom;

    // tok.mod == "."
    if (tok.mod == Modifier::Once) {
        if (textIdx >= text.size() || !atom.matches(text[textIdx])) {
            return false;
        }
        
        return matchFrom(tokIdx + 1, text, textIdx + 1);
    }

    if (tok.mod == Modifier::Star || tok.mod == Modifier::Plus) {
        // * и + жадные моды: подсчет сколько символов в тексте подходит подряд
        int count = 0;
        while (textIdx + count < text.size() && atom.matches(text[textIdx + count])) {
            ++count;
        }

        if (tok.mod == Modifier::Plus && count == 0) {
            return false;
        }

        return matchFrom(tokIdx + 1, text, textIdx + count);
    }

    if (tok.mod == Modifier::Question){
        // попытка взять 0 символов
        if (matchFrom(tokIdx + 1, text, textIdx)) {
            return true;
        }
        // 2 вариант - 1 символ
        if (textIdx < text.size() && atom.matches(text[textIdx])) {
            return matchFrom(tokIdx + 1, text, textIdx + 1);
        }

        return false;
    }
}

bool Regex::match(const std::string& text) {
    return matchFrom(0, text, 0);
}