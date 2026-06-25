#pragma once

#include <memory>
#include <string>
#include <vector>

class Atom {
public:
    virtual bool matches(char c) const = 0;
    virtual ~Atom() = default;
};

// конкретный символ: "a", "B", "3", " "
class LiteralAtom : public Atom {
private:
    char ch;
public:
    LiteralAtom(char c) : ch(c) {}
    bool matches(char c) const override { return c == ch; }
};

// любой символ: "."
class AnyAtom : public Atom {
public:
    bool matches(char) const override { return true; }
};

// группа символов: "[abc]"", "[a-z]
class GroupAtom : public Atom {
private:
    std::string chars;
public:
    GroupAtom(std::string chars2) : chars(chars2) {}
    bool matches(char c) const override {
        return chars.find(c) != std::string::npos;
    }
};

enum Modifier { 
    Once, 
    Star, 
    Plus, 
    Question,
};

struct Token {
    std::unique_ptr<Atom> atom;
    Modifier mod;
};

class Regex {
private:
    std::vector<Token> tokens;
public:
    Regex(const std::string& pattern);
    bool match(const std::string& text);
    bool matchFrom(int tokIdx, const std::string& text, int textIdx);
};