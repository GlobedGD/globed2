#pragma once
#define GLOBED_LAZY(expr) ::globed::lazyExpr([&]() -> decltype(expr) { return expr; })

namespace globed {
    template <typename F, typename Ret = std::invoke_result_t<F>>
    class LazyExpr {
        std::variant<F, Ret> value;
        Ret& _eval() {
            if (std::holds_alternative<F>(value)) {
                value = std::get<F>(value)();
            }

            return std::get<Ret>(value);
        }

    public:
        LazyExpr(F&& f) : value(std::forward<F>(f)) {} // idk if the forward is correct here

        Ret& operator*() {
            return this->_eval();
        }

        Ret& operator->() {
            return this->_eval();
        }

        Ret& getLazyValue() {
            return this->_eval();
        }
    };

    // Returns a type that will only evaluate the given expression when it's referenced
    template <typename F>
    auto lazyExpr(F&& f) {
        return LazyExpr<F>(std::forward<F>(f));
    }
}