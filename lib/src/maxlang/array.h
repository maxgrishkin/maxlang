#pragma once

#include "value.h"
#include <vector>
#include <memory>
#include <string>
#include <iostream>

namespace maxlang {

    struct Array {
        std::vector<Value> elements;
        std::string name; // Имя для отладки

        Array() = default;
        explicit Array(const std::string& name) : name(name) {}
        explicit Array(std::vector<Value> elements, const std::string& name = "")
            : elements(std::move(elements)), name(name) {}

        Value& operator[](size_t index) {
            if (index >= elements.size()) {
                throw std::runtime_error("Array index out of bounds");
            }
            return elements[index];
        }

        const Value& operator[](size_t index) const {
            if (index >= elements.size()) {
                throw std::runtime_error("Array index out of bounds");
            }
            return elements[index];
        }

        size_t size() const { return elements.size(); }
        void push_back(const Value& value) { elements.push_back(value); }
        void pop_back() {
            if (elements.empty()) {
                throw std::runtime_error("Cannot pop from empty array");
            }
            elements.pop_back();
        }
    };
    void printArray(std::ostream& os, const Array& array);
    // Объявите оператор вывода внутри пространства имен
    std::ostream& operator<<(std::ostream& os, const Array& array);

}