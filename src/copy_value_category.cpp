#include <type_traits>

namespace detail {

template <class From, class To>
[[nodiscard]] constexpr auto do_copy_vcat() noexcept {
    using decayed_to = std::remove_reference_t<To>;

    constexpr bool is_adding_const =
        std::is_const_v<std::remove_reference_t<From>>;
    if constexpr (std::is_lvalue_reference_v<From>) {
        if constexpr (is_adding_const)
            return std::type_identity<const decayed_to &>{};
        else
            return std::type_identity<decayed_to &>{};
    } else if constexpr (std::is_rvalue_reference_v<From>) {
        if constexpr (is_adding_const)
            return std::type_identity<const decayed_to &&>{};
        else
            return std::type_identity<decayed_to &&>{};
    } else {
        if constexpr (is_adding_const)
            return std::type_identity<const decayed_to>{};
        else
            return std::type_identity<decayed_to>{};
    }
}

}  // namespace detail

template <typename From, typename To>
struct copy_value_category {
    using type = decltype(detail::do_copy_vcat<From, To>())::type;
};

template <typename From, typename To>
using copy_value_category_t = typename copy_value_category<From, To>::type;

template <typename From, typename To, typename Expected>
constexpr void test() noexcept {
    static_assert(std::is_same_v<copy_value_category_t<From, To>, Expected>);
}

int main() {
    test<int, int, int>();
    test<int, long, long>();

    test<int &, long, long &>();
    test<const int &, long, const long &>();

    test<int &&, long, long &&>();
    test<const int &, long, const long &>();

    test<int *, long, long>();
    test<const int *, long, long>();
    test<int *const, long, const long>();
    test<const int *const, long, const long>();

    // Additional:
    test<int, long &, long>();
    test<int, const long &, const long>();
    test<int &, const long &, const long &>();
    test<int &&, const long &, const long &&>();

    test<int, long &&, long>();
    test<int, const long &&, const long>();
    test<int &, const long &&, const long &>();
    test<int &&, const long &&, const long &&>();
    test<const int &, const long &&, const long &>();
    test<const int &&, const long &&, const long &&>();
    test<const int &, long &&, const long &>();
    test<const int &&, long &&, const long &&>();

    // Arrays
    test<int[], int, int>();
    test<int[], long, long>();
    test<int, long[], long[]>();

    test<int(&)[], int, int &>();
    test<int(&)[], long, long &>();
    test<int(&&)[], int, int &&>();
    test<int(&&)[], long, long &&>();
}
