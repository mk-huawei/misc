#include <fmt/format.h>

#include <boost/mp11/map.hpp>
#include <boost/mp11/utility.hpp>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

using namespace boost::mp11;

namespace mk {
namespace detail {

template <typename...>
struct reverse_pair_impl;

template <template <typename...> class Pair, class T1, class T2, class... Rest>
struct reverse_pair_impl<Pair<T1, T2, Rest...>> {
    using type = Pair<T2, T1, Rest...>;
};

}  // namespace detail

template <typename Pair>
using reverse_pair = typename detail::reverse_pair_impl<Pair>::type;

template <class M>
using reverse_map = mp_transform<reverse_pair, M>;

}  // namespace mk

using forw_map = mp_list<                //
    std::pair<int, mp_identity<int>>,    //
    std::pair<long, mp_identity<long>>,  //
    std::pair<short, mp_identity<short>>>;

using back_map = mk::reverse_map<forw_map>;

int main() {
    static_assert(mp_is_map<forw_map>::value);
    static_assert(mp_size<forw_map>::value == 3);

    static_assert(!mp_map_contains<forw_map, void>::value);
    static_assert(std::is_same_v<mp_map_find<forw_map, void>, void>);

    static_assert(mp_map_contains<forw_map, int>::value);
    static_assert(!std::is_same_v<mp_map_find<forw_map, int>, void>);
    static_assert(
        std::is_same_v<typename mp_map_find<forw_map, int>::second_type,
                       mp_identity<int>>);
    static_assert(
        std::is_same_v<typename mp_map_find<forw_map, short>::second_type,
                       mp_identity<short>>);

    static_assert(!mp_map_contains<forw_map, mp_identity<int>>::value);
    static_assert(
        std::is_same_v<mp_map_find<forw_map, mp_identity<int>>, void>);

    // Backwards:
    static_assert(mp_is_map<back_map>::value);
    static_assert(mp_size<back_map>::value == 3);

    static_assert(!mp_map_contains<back_map, void>::value);
    static_assert(std::is_same_v<mp_map_find<back_map, void>, void>);

    static_assert(!mp_map_contains<back_map, int>::value);
    static_assert(std::is_same_v<mp_map_find<back_map, int>, void>);

    static_assert(mp_map_contains<back_map, mp_identity<int>>::value);
    static_assert(
        !std::is_same_v<mp_map_find<back_map, mp_identity<int>>, void>);
    static_assert(std::is_same_v<
                  typename mp_map_find<back_map, mp_identity<int>>::second_type,
                  int>);
    static_assert(
        std::is_same_v<
            typename mp_map_find<back_map, mp_identity<short>>::second_type,
            short>);
}
