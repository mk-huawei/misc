// Source: https://godbolt.org/z/qnTdozoo6, https://schedule.cppnow.org/wp-content/uploads/2025/03/CNow-Advanced-Ranges.pdf

#include <ranges>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <iterator>

template <std::ranges::view InputView>
class stock_threshold_view : public std::ranges::view_interface<stock_threshold_view<InputView>> {
private:
    InputView base_;
    std::unordered_map<std::string, double> const& stock_data_;
    double threshold_;

    // Iterator class that handles filtering
    class iterator {
    public:
        // Iterator traits
        using base_iterator = std::ranges::iterator_t<InputView>;
        using iterator_concept = std::input_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ranges::range_difference_t<InputView>;
        using value_type = std::ranges::range_value_t<InputView>;
        using reference = std::ranges::range_reference_t<InputView>;

        // Default constructor
        iterator() = default;

        // Constructor
        iterator(base_iterator current, base_iterator end, 
                 std::unordered_map<std::string, double> const* stock_data, 
                 double threshold)
            : current_(current), end_(end), 
              stock_data_(stock_data), threshold_(threshold) {
            // Find the first valid element
            find_next_valid();
        }

        const auto& operator*() const {
            return *current_;
        }

        // Pre-increment
        iterator& operator++() {
            if (current_ != end_) {
                ++current_;
                find_next_valid();
            }
            return *this;
        }

        // Post-increment
        void operator++(int) {
            ++(*this);
        }

        // Equality comparison
        bool operator==(const iterator& other) const {
            return current_ == other.current_;
        }
    private:
        base_iterator current_{};
        base_iterator end_{};
        std::unordered_map<std::string, double> const* stock_data_{};
        double threshold_{};

        // Helper method to find the next valid element
        void find_next_valid() {
            while (current_ != end_) {
                const auto& stock = *current_;
                if (stock_data_->contains(stock) && (*stock_data_).at(stock) >= threshold_) {
                    return;
                }
                ++current_;
            }
        }
    };

public:
    // Constructor
    stock_threshold_view(InputView base, 
                         std::unordered_map<std::string, double> const& stock_data, 
                         double threshold)
        : base_(std::move(base)), stock_data_(stock_data), threshold_(threshold) {}

    // Begin iterator
    auto begin() {
        return iterator(std::ranges::begin(base_), std::ranges::end(base_),
                       &stock_data_, threshold_);
    }

    // End iterator
    auto end() {
        return iterator(std::ranges::end(base_), std::ranges::end(base_),
                       &stock_data_, threshold_);
    }
};

// Deduction guide
template <class R>
stock_threshold_view(R&&, 
                    const std::unordered_map<std::string, double>&, 
                    double) 
    -> stock_threshold_view<std::views::all_t<R>>;

namespace views {
    // This adaptor closure holds the external state and inherits from range_adaptor_closure
    struct stock_threshold_closure : public std::ranges::range_adaptor_closure<stock_threshold_closure> {
        std::unordered_map<std::string, double> const& stock_data;
        double threshold;
        
        constexpr stock_threshold_closure(
            std::unordered_map<std::string, double> const& data, 
            double t) 
            : stock_data(data), threshold(t) {}
        
        // The call operator creates the view
        template <std::ranges::viewable_range R>
        constexpr auto operator()(R&& r) const {
            return stock_threshold_view(std::views::all(std::forward<R>(r)), stock_data, threshold);
        }
    };
    
    // The factory function
    inline constexpr auto stock_threshold(
        std::unordered_map<std::string, double> const& stock_data, 
        double threshold) {
        return stock_threshold_closure(stock_data, threshold);
    }
}

// Example usage
int main() {
    // List of stocks we are monitoring
    std::vector<std::string> stocks = {"MSFT", "TSLA", "IBM", "NVDA", "PEP", "AML", "AAPL", "APP"};
    
    // External data: stream of data
    // Stocks mapped to some threshold (e.g. Sharpe Ratio)
    std::unordered_map<std::string, double> stock_data = {
        {"MSFT", 1.1},
        {"TSLA", 1.0},
        {"NVDA", 1.6},
        {"PEP", 1.8},
        {"AAPL", 2.1},
        {"APP", 2.3} 
    };

    double threshold = 1.5;
    
    auto pipeline = stocks | views::stock_threshold(stock_data, threshold);
    
    std::cout << "Stocks above threshold: ";
    for (const auto& stock : pipeline) {
        std::cout << stock << ", "; // NVDA, PEP, AAPL, APP
    }
    std::cout << std::endl;

    stock_data["AAPL"] = 0.5;
    
    std::cout << "After changing AAPL value: ";
    for (const auto& stock : pipeline) {
        std::cout << stock << ", "; // NVDA, PEP, APP
    }
    std::cout << std::endl;

    return 0;
}
