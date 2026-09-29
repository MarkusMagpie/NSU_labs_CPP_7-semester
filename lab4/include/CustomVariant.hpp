#pragma once

#include <algorithm> // std::max
#include <cstddef> // std::size_t
#include <new> // placement new, std::launder
#include <stdexcept> // std::logic_error
#include <type_traits>  // std::is_same_v
#include <utility> // std::move

namespace lab4 {

// бросается из get<T>() если в варианте лежит не T (аналог std::bad_variant_access)
class BadVariantAccess : public std::logic_error {
public:
    BadVariantAccess() : std::logic_error("CustomVariant: запрошен не тот тип, который хранится") {}
};



template <typename... Types>
class CustomVariant {
private:
    alignas(Types...) unsigned char buffer_[std::max({sizeof(Types)...})]; // sizeof(Types)=sizeof(bool), sizeof(int), ...
    // номер типа который сейчас лежит в buffer_
    std::size_t index_;
public:
    // на каком месте тип T стоит в списке Types? (bool=0,int=1,...)
    // если T в списке нет -> к-во типов
    template <typename T>
    static constexpr std::size_t index_of() {
        bool matches[] = { std::is_same_v<T, Types>... }; // bool array
        for (std::size_t i = 0; i < sizeof...(Types); ++i) {
            if (matches[i] ) {
                return i;
            }
        }

        return sizeof...(Types); // кол-во типов в списке
    }

    // конструктор из значения: CustomVariant<bool, int, std::string> v(42); -> внутри лежит int
    // T выводится из типа аргумента
    template <typename T>
    CustomVariant(T value) {
        static_assert(index_of<T>() < sizeof...(Types), "CustomVariant: этого типа нет в списке Types");

        // new T(std::move(value));
        new (buffer_) T(std::move(value)); // память не выделять а создать объект value в buffer_
        index_ = index_of<T>();
    }

    // копирующий конструктор CustomVariant a = b;
    CustomVariant(const CustomVariant& other) : index_(other.index_) {
        (copy_if<Types>(other), ...);
    }

    ~CustomVariant() {
        destroy();
    }

    // какой тип хранится сейчас (номер в списке Types)
    [[nodiscard]] std::size_t index() const {
        return index_;
    }

    // лежит ли сейчас тип T? (std::holds_alternative 2.0)
    template <typename T>
    [[nodiscard]] bool holds_alternative() const {
        static_assert(index_of<T>() < sizeof...(Types), "CustomVariant: этого типа нет в списке Types");

        return index_ == index_of<T>();
    }

    // достать значение типа T (std::get 2.0); если другой тип -> BadVariantAccess
    template <typename T>
    T& get() {
        if (!holds_alternative<T>()) {
            throw BadVariantAccess();
        }

        // байты buffer_ как объект T, который был положен туда в конструкторе
        return *reinterpret_cast<T*>(buffer_);
    }

    template <typename T>
    const T& get() const {
        if (!holds_alternative<T>()) {
            throw BadVariantAccess();
        }

        return *reinterpret_cast<const T*>(buffer_);
    }

    // уничтожить объект в buffer_ если там лежит T
    template <typename T>
    void destroy_if() {
        if (index_ == index_of<T>()) {
            reinterpret_cast<T*>(buffer_)->~T(); // явный вызов деструктора T
        }
    }

    // уничтожить объект в buffer_
    void destroy() {
        (destroy_if<Types>(), ...);
    }

    // если в other лежит T -> создать в своем buffer_ копию объекта T
    template <typename T>
    void copy_if(const CustomVariant& other) {
        if (other.index_ == index_of<T>()) {
            new (buffer_) T(other.get<T>()); // копирующий конструктор T: у копии свои данные
        }
    }
};

} // namespace lab4