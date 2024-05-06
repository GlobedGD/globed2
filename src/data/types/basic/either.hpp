#pragma once

#include <variant>
#include <optional>
#include <utility>

template <typename T, typename Y>
class Either {
public:
    static_assert(!std::is_same_v<T, Y>, "Template arguments for Either must be different types");

    using first_type = T;
    using second_type = Y;

    Either(const T& t) : variant(t) {}
    Either(T&& t) : variant(std::move(t)) {}
    Either(const Y& y) : variant(y) {}
    Either(Y&& y) : variant(std::move(y)) {}

    Either(const Either&) = default;
    Either& operator=(const Either&) = default;
    Either(Either&&) = default;
    Either& operator=(Either&&) = default;

    bool isFirst() const {
        return std::holds_alternative<T>(variant);
    }

    bool isSecond() const {
        return !isFirst();
    }

    T&& unwrapFirst() {
        return std::move(std::get<T>(variant));
    }

    Y&& unwrapSecond() {
        return std::move(std::get<Y>(variant));
    }

    std::optional<T> first() {
        if (this->isSecond()) {
            variant = {};
            return std::nullopt;
        } else {
            return std::optional(std::move(this->unwrapFirst()));
        }
    }

    std::optional<Y> second() {
        if (this->isFirst()) {
            variant = {};
            return std::nullopt;
        } else {
            return std::optional(std::move(this->unwrapSecond()));
        }
    }

    std::optional<std::reference_wrapper<T>> firstRef() {
        return this->isSecond() ? std::nullopt : std::optional(std::ref(std::get<T>(variant)));
    }

    std::optional<std::reference_wrapper<Y>> secondRef() {
        return this->isFirst() ? std::nullopt : std::optional(std::ref(std::get<Y>(variant)));
    }

    std::optional<std::reference_wrapper<const T>> firstRef() const {
        return this->isSecond() ? std::nullopt : std::optional(std::ref(std::get<T>(variant)));
    }

    std::optional<std::reference_wrapper<const Y>> secondRef() const {
        return this->isFirst() ? std::nullopt : std::optional(std::ref(std::get<Y>(variant)));
    }

private:
    std::variant<T, Y> variant;
};

