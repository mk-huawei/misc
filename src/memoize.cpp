#include <cstdint>
#include <print>
#include <tuple>
#include <unordered_map>

namespace std {
template <typename T, typename... Ts>
struct hash<tuple<T, Ts...>> {
    static size_t operator()(const tuple<T, Ts...> &v) noexcept {
        return std::hash<T>{}(std::get<0>(v));
    }
};
}  // namespace std

template <typename F>
auto simple_memoize(F &&f) {
    return [f = std::forward<F>(f)]<typename... Args>(this auto self,
                                                      Args... args) {
        using key_type = std::tuple<std::decay_t<Args>...>;
        using value_type = decltype(f(std::forward<Args>(args)...));
        static std::unordered_map<key_type, value_type> memoized;

        // std::println("size(hm) = {}", std::size(memoized));

        auto args_tuple = std::make_tuple(args...);
        if (auto it = memoized.find(args_tuple); it != memoized.end()) {
            std::println("found fib({}) = {}", args_tuple, it->second);
            return it->second;
        }

        auto res = f(std::forward<Args>(args)...);
        // std::println("emplace fib({}) = {}", args_tuple, res);
        memoized.emplace(args_tuple, res);
        return res;
    };
}

uint64_t fib(uint64_t n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

auto fibo = [](this auto self, uint64_t n) -> uint64_t {
    if (n <= 1) {
        return n;
    }
    return self(n - 1) + self(n - 2);
};

int main() {
    std::println("fib(20) = {}", fib(20));
    std::println("fib(15) = {}", fib(15));
    std::println("fib(22) = {}", fib(22));
    std::println("fibo(20) = {}", fibo(20));
    std::println("fibo(15) = {}", fibo(15));
    std::println("fibo(22) = {}", fibo(22));

    auto mfib = simple_memoize(fib);
    std::println("mem fib(20) = {}", mfib(20));
    std::println("mem fib(15) = {}", mfib(15));
    std::println("mem fib(22) = {}", mfib(22));
    std::println("mem fib(22) = {}", mfib(22));
}
