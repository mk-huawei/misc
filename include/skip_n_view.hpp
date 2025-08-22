// Source: https://godbolt.org/z/bnbnq8rWs, https://godbolt.org/z/1je7nfn8c, https://schedule.cppnow.org/wp-content/uploads/2025/03/CNow-Advanced-Ranges.pdf

#include <ranges>
#include <vector>
#include <iostream>

template <std::ranges::view InputView> requires std::ranges::common_range<InputView>
class skip_n_view : public std::ranges::view_interface<skip_n_view<InputView>> {
private:
    InputView input_range_;
    size_t n_;
public:
    class iterator { 
    public:
        //iterator type traits
        using base_iterator = std::ranges::iterator_t<InputView>;
        using iterator_category = std::input_iterator_tag;
        using iterator_concept = std::input_iterator_tag; 
        using value_type = std::ranges::range_value_t<InputView>;
        using difference_type = std::ranges::range_difference_t<InputView>; 
        using reference = std::ranges::range_reference_t<InputView>; 

        iterator() = default;
        iterator(base_iterator current, base_iterator end, std::size_t n)
            : current_position_(current), end_(end), n_(n) {}

        auto operator*() const {
            return *current_position_;
        }

        iterator& operator++() {
            current_position_++;
            counter_++;
            if (current_position_ != end_ && (counter_ % n_ == 0)) {
                current_position_++;
                counter_++;
            }
            return *this;
        }

        bool operator==(const iterator& rhs) const {
            return current_position_ == rhs.current_position_;
        }

    private:
        base_iterator current_position_;
        base_iterator end_;
        std::size_t n_;
        std::size_t counter_ = 1;
    };

    skip_n_view() = default;
    constexpr skip_n_view(InputView input_range, size_t n) : input_range_(input_range), n_(n) {}

    constexpr iterator begin() const {
        return iterator {
            std::ranges::begin(input_range_), std::ranges::end(input_range_), n_
        };
     }
    constexpr iterator end() const {
        return iterator {
            std::ranges::end(input_range_), std::ranges::end(input_range_), n_
        };  
    }
};

// Deduction guide
template <typename R>
skip_n_view(R&&, std::size_t) -> skip_n_view<std::views::all_t<R>>;

namespace views {
    // Range adaptor closure - inherits from std::ranges::range_adaptor_closure
    struct skip_n_closure : std::ranges::range_adaptor_closure<skip_n_closure> {
        std::size_t n;
        
        constexpr skip_n_closure(std::size_t n_val) : n(n_val) {}
        
        template <std::ranges::viewable_range R>
        constexpr auto operator()(R&& r) const {
            return skip_n_view{std::forward<R>(r), n};
        }
    };

    // Range adaptor object
    struct skip_n_t {
        constexpr skip_n_closure operator()(std::size_t n) const {
            return skip_n_closure(n);  // Using constructor instead of aggregate initialization
        }
    };

    // The actual object instance
    inline constexpr skip_n_t skip_n{};
}

int main() {
    std::vector<int> data {1, 4, 2, 8, 9, 11, 12, 14, 18};
    
    auto pipeline = data | views::skip_n(3);
    for (auto elem : pipeline) {
        std::cout << elem << " "; // 1 4 8 9 12 14
    }
    return 0;
}
