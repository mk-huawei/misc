// Source: https://godbolt.org/z/1je7nfn8c, https://schedule.cppnow.org/wp-content/uploads/2025/03/CNow-Advanced-Ranges.pdf

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

                iterator operator++(int) {
                    auto tmp = *this;
                    ++(*this);
                    return tmp;
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

int main() {
    std::vector<int> data {1, 4, 2, 8, 9, 11, 12, 14, 18};
    std::size_t n = 3;
    skip_n_view test{std::views::all(data), n};

    for (auto elem : test) {
        std::cout << elem << " "; // // 1 4 8 9 12 14
    }

    std::cout << std::endl;
    return 0;
}
