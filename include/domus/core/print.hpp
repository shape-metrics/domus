#ifndef PRINT_HPP
#define PRINT_HPP

#include <iostream>
#include <string>
#include <vector>

template <typename Iterable>
void print_iterable(const Iterable& container, const std::string& end = "\n") {
    std::cout << "[ ";
    for (const auto& elem : container)
        std::cout << elem << " ";
    std::cout << "]" << end;
}

template <typename T>
concept Printable = requires(const T& t) {
    { t.print() } -> std::same_as<void>;
    { t.to_string() } -> std::same_as<std::string>;
};

template <Printable T> void print_vector_of_printable(const std::vector<T>& container) {
    for (const auto& elem : container)
        elem.print();
}

template <Printable T> std::string to_string_vector_of_printable(const std::vector<T>& container) {
    std::string result{};
    for (const auto& elem : container)
        result += elem.to_string() + "\n";
    return result;
}

template <typename T> void print_array(T array[], const int size, const std::string& end = "\n") {
    std::cout << "[ ";
    for (int i = 0; i < size; ++i)
        std::cout << array[i] << " ";
    std::cout << "]" << end;
}

#endif
