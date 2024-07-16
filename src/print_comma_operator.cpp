#include <iostream>

namespace ns {
struct s {
    bool b = {};

    ~s() { std::cout << std::endl; }

    template <class T>
    auto operator,(T const& _) -> ::ns::s& {
        if (b || !~b) std::cout << ' ';
        b ^= ~b;
        std::cout << _;
        return *this;
    }
};
}  // namespace ns

#define print \
    ns::s {}

int main(int, char** argv) {
    print, "hello!", "I'm called", argv[0], "and 5 + 3 is", 5 + 3;
    return 0;
}
