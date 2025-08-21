# Destructuring zip_view's iterator

We propose to add a way to portably destructure an iterator coming from a `zip_view`.

## Motivation

In some cases, we want to erase elements from `views::zip`'ped ranges.

C++26:
```
auto abc = std::views::zip(a, b, c);
std::ranges::subrange to_erase = std::ranges::remove_if(abc, condition);
using std::ranges::begin;
using std::ranges::end;
auto dist = std::ranges::distance(begin(a), to_erase.begin());
// We should actually use end_dist, because zipped ranges might have different lengths.
a.erase(begin(a) + dist, end(a));
b.erase(begin(b) + dist, end(b));
c.erase(begin(c) + dist, end(c));
```

Proposed:
```
auto abc = std::views::zip(a, b, c);
std::ranges::subrange to_erase = std::ranges::remove_if(abc, condition);
auto [...its] = to_erase.begin();
auto [...ends] = to_erase.end();

a.erase(its...[0], ends...[0]);
b.erase(its...[1], ends...[1]);
c.erase(its...[2], ends...[2]);
// or
/*
auto [...rngs] = abc;
constexpr int i = 0;
template for (auto & rng : rngs) {
    rng.erase(its...[i], ends...[i]);
}
*/
```

Or we just need to access one of the zipped iterators.

## Current status

The user can do the desired thing when using libc++ and its implentation-specific `__product_iterator_traits` class, as shown in https://godbolt.org/z/TYoxPadvc:

```
#include <print>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

int main() {
    namespace views = std::views;

    [[maybe_unused]] std::vector<int> v = {1, 2, 3, 4};
    [[maybe_unused]] std::vector<int> w = {2, 3, 5, 7};
    auto v2 = views::zip(v, w);
    std::println("v2: {}", v2);

    auto it = std::ranges::begin(v2) + 1;
    std::println("it: {}", *it);

#ifdef _LIBCPP_VERSION  // libc++
    using pit = std::__product_iterator_traits<decltype(it)>;
    // also: __zip_view_iterator_access
    auto it0 = pit::__get_iterator_element<0>(it);
    std::println("it0: {}", *it0);
#endif

    // auto [...its] = it;
    // error: cannot decompose private member '__current_' of 'std::ranges::zip_view<std::ranges::ref_view<std::vector<int>>, std::ranges::ref_view<std::vector<int>>>::__iterator<true>'
}
```

This is however not portable and currently impossible to achieve using libstdc++.

Both implementation of the standard library use however a tuple of underlying iterators in their implementation.

## Proposal

TODO
