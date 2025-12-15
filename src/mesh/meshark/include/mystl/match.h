#pragma once

#include <utility>
#include <variant>

template <typename... Ts> struct Visitor : Ts... {
    using Ts::operator()...;
};

template <typename... Ts> Visitor(Ts...) -> Visitor<Ts...>;  // 辅助编译器推导类型

template <typename T> struct Match {
    T value;
    Match(T&& value) : value(std::forward<T>(value)) {}
    template <typename... Ts> auto operator()(Ts&&... params) {
        return std::visit(Visitor{std::forward<Ts>(params)...}, std::forward<T>(value));
    }
    template <typename... Ts> auto operator()(Visitor<Ts...> visitor) {
        return std::visit(visitor, std::forward<T>(value));
    }
};

template <typename T> Match(T&&) -> Match<T&&>;
