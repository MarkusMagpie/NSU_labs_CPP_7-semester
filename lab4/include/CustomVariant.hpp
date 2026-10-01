#pragma once

#include <algorithm> // std::max
#include <cstddef> // std::size_t
#include <new> // placement new, std::launder
#include <stdexcept> // std::logic_error
#include <type_traits>  // std::is_same_v
#include <utility> // std::move
#include <concepts> // std::same_as

namespace lab4 {

// бросается из get<T>() если в варианте лежит не T (аналог std::bad_variant_access)
class BadVariantAccess : public std::logic_error {
public:
    BadVariantAccess() : std::logic_error("CustomVariant: запрошен не тот тип, который хранится") {}
};



/**
   * @brief Упрощенный аналог std::variant: хранит ровно одно значение одного из типов Types...
   *
   * Значение лежит внутри объекта, в буфере buffer_, без выделения памяти в куче.
   * Поле index_ хранит индекс типа который там лежит сейчас (номер типа в списке Types).
   * Сам список типов во время работы не хранится: его знает только компилятор.
   *
   * @tparam Types список допустимых типов данных. Например: bool, int, std::string.
   */
template <typename... Types>
class CustomVariant {
private:
    // сырые байты под одно значение типа T
    alignas(Types...) unsigned char buffer_[std::max({sizeof(Types)...})];
    // номер типа который сейчас лежит в buffer_
    std::size_t index_;
public:
    /**
     * @brief Номер типа T в списке Types. Вычисляется при компиляции.
     *
     * Пример: для CustomVariant<bool, int, std::string> index_of<int>() == 1.
     *
     * @tparam T тип, который ищется
     * @return позиция T в Types; если T в списке нет, возвращает количество типов данных (sizeof...(Types))
     */
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
    // T выводится из типа аргумента; requires - конструктор существует только для T из списка Types
    template <typename T> requires (std::same_as<T, Types> || ...)
    CustomVariant(T value) {
        // new T(std::move(value));
        new (buffer_) T(std::move(value)); // память не выделять; создать объект value в байтах buffer_
        index_ = index_of<T>();
    }

    // копирующий конструктор CustomVariant a = b; 1 - скопировать index_; 2 - содержимое буфера
    CustomVariant(const CustomVariant& other) : index_(other.index_) {
        (copy_if<Types>(other), ...);
    }

    /**
     * @brief Если в other лежит тип T, то метод создает в своем buffer_ копию этого объекта.
     *
     * @tparam T тип из списка Types, который сравнивается с типом, хранящимся в other
     * @param other объект CustomVariant, из buffer_ которого берется объект, копия которого строится в моем buffer_
     */
    template <typename T>
    void copy_if(const CustomVariant& other) {
        if (other.index_ == index_of<T>()) {
            new (buffer_) T(other.get<T>()); // копирующий конструктор T: у копии свои данные
        }
    }

    ~CustomVariant() {
        destroy();
    }

    // какой тип хранится сейчас (номер в списке Types)
    [[nodiscard]] std::size_t index() const {
        return index_;
    }

    // лежит ли сейчас тип T? (std::holds_alternative 2.0)
    template <typename T> requires (std::same_as<T, Types> || ...)
    [[nodiscard]] bool holds_alternative() const {
        return index_ == index_of<T>();
    }

    // достать значение типа T (std::get 2.0); если другой тип -> BadVariantAccess
    template <typename T> requires (std::same_as<T, Types> || ...)
    T& get() {
        if (!holds_alternative<T>()) {
            throw BadVariantAccess();
        }

        // байты buffer_ как объект T, который был положен туда в конструкторе
        return *reinterpret_cast<T*>(buffer_);
    }

    template <typename T> requires (std::same_as<T, Types> || ...)
    const T& get() const {
        if (!holds_alternative<T>()) {
            throw BadVariantAccess();
        }

        return *reinterpret_cast<const T*>(buffer_);
    }

    /**
     * @brief Если в buffer_ лежит объект типа T, то явно вызывается деструктор ~T().
     *
     *
     * @tparam T тип из списка Types, который сравнивается с типом, хранящимся сейчас.
     *  Если совпал, вызывается ~T(); если нет, ничего не происходит
     */
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
};

} // namespace lab4