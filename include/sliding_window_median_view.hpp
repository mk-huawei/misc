// Source: https://godbolt.org/z/9bGaz6GGz, https://schedule.cppnow.org/wp-content/uploads/2025/03/CNow-Advanced-Ranges.pdf

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <iostream>
#include <random>
#include <ranges>
#include <vector>

// Sliding median view using std::deque
template <typename InputView>
requires std::ranges::common_range<InputView>
class sliding_median_view_deque : public std::ranges::view_interface<sliding_median_view_deque<InputView>> {
private:
    InputView input_range_;
    std::size_t window_size_;
    
public:
    class iterator {
    public:
        // Iterator type traits
        using base_iterator = std::ranges::iterator_t<InputView>;
        using iterator_category = std::input_iterator_tag;
        using iterator_concept = std::input_iterator_tag;
        
        using value_type = std::ranges::range_value_t<InputView>;
        using difference_type = std::ranges::range_difference_t<InputView>;
        
        using reference = value_type; // Return by value for the median
        
        iterator() = default;
        
        iterator(base_iterator begin, base_iterator end, std::size_t window_size)
            : current_(begin), end_(end), window_size_(window_size) {
            
            if (begin == end) {
                return; 
            }
            
            std::size_t count = 0;
            auto it = begin;
            while (it != end && count < window_size) {
                window_.push_back(*it);
                ++it;
                ++count;
            }
            
            if (count < window_size) {
                current_ = end_;
            }
        }
        
        auto operator*() const {            
            std::vector<value_type> sorted(window_.begin(), window_.end());
            const size_t mid_idx = sorted.size() / 2;
            
            std::nth_element(sorted.begin(), sorted.begin() + mid_idx, sorted.end());
            
            if (sorted.size() % 2 == 0) {
                // Even number of elements - find the other middle element
                // First, find the max of the lower half
                auto max_it = std::max_element(sorted.begin(), sorted.begin() + mid_idx);
                auto lower_median = *max_it;
                auto higher_median = sorted[mid_idx];
                return (lower_median + higher_median) / 2;
            } else {
                // Odd number of elements - return the middle one
                return sorted[mid_idx];
            }
        }
        
        iterator& operator++() {
            ++current_;
            
            if (current_ + window_size_ > end_) {
                current_ = end_;
                window_.clear();
                return *this;
            }
            
            window_.pop_front();
            window_.push_back(*(current_ + window_size_ - 1));
            
            return *this;
        }
        
        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        
        bool operator==(const iterator& rhs) const {
            return current_ == rhs.current_;
        }
        
    private:
        base_iterator current_;
        base_iterator end_;
        std::size_t window_size_;
        std::deque<value_type> window_;
    };
    
    sliding_median_view_deque() = default;
    
    constexpr sliding_median_view_deque(InputView input_range, std::size_t window_size)
        : input_range_(input_range), window_size_(window_size) {}
    
    constexpr iterator begin() const {
        return iterator {
            std::ranges::begin(input_range_), 
            std::ranges::end(input_range_),
            window_size_
        };
    }
    
    constexpr iterator end() const {
        return iterator {
            std::ranges::end(input_range_),
            std::ranges::end(input_range_),
            window_size_
        };
    }
};

// Deduction guide for sliding_median_view_deque
template <typename R>
sliding_median_view_deque(R&&, std::size_t) -> sliding_median_view_deque<std::views::all_t<R>>;

namespace views {
    struct sliding_median_deque_fn : public std::ranges::range_adaptor_closure<sliding_median_deque_fn> {
        std::size_t window_size;
        
        constexpr explicit sliding_median_deque_fn(std::size_t window_size) 
            : window_size(window_size) {}
            
        template <std::ranges::viewable_range R>
        constexpr auto operator()(R&& r) const {
            return sliding_median_view_deque{std::views::all(std::forward<R>(r)), window_size};
        }
    };
    
    constexpr auto sliding_median_deque(std::size_t window_size) {
        return sliding_median_deque_fn{window_size};
    }
}

// Sliding median view using std::array with circular buffer
template <typename InputView, std::size_t WindowSize>
requires std::ranges::common_range<InputView>
class sliding_median_view_array : public std::ranges::view_interface<sliding_median_view_array<InputView, WindowSize>> {
private:
    InputView input_range_;
    
public:
    class iterator {
    public:
        using base_iterator = std::ranges::iterator_t<InputView>;
        using iterator_category = std::input_iterator_tag;
        using iterator_concept = std::input_iterator_tag;
        
        using value_type = std::ranges::range_value_t<InputView>;
        using difference_type = std::ranges::range_difference_t<InputView>;
        
        using reference = value_type;
        /*...*/
        
        iterator() = default;
        
        iterator(base_iterator begin, base_iterator end)
            : current_(begin), end_(end) {
            
            if (begin == end) {
                return;
            }
            
            auto count = 0;
            auto it = begin;
            while (it != end && count < WindowSize) {
                window_[count] = *it;
                ++it;
                ++count;
            }
            
            if (count < WindowSize) {
                current_ = end_;
            }
            
            window_size_ = count;
            oldest_idx_ = 0;
        }
        
        // Return median of current window
        auto operator*() const {
            
            std::array<value_type, WindowSize> sorted;
            std::copy_n(window_.begin(), window_size_, sorted.begin());
            
            const size_t mid_idx = window_size_ / 2;
            
            std::nth_element(sorted.begin(), sorted.begin() + mid_idx, sorted.begin() + window_size_);
            
            if (window_size_ % 2 == 0) {
                auto max_it = std::max_element(sorted.begin(), sorted.begin() + mid_idx);
                auto lower_median = *max_it;
                auto higher_median = sorted[mid_idx];
                return (lower_median + higher_median) / 2;
            } else {
                return sorted[mid_idx];
            }
        }
        
        iterator& operator++() {
            ++current_;
            
            if (current_ + WindowSize - 1 >= end_) {
                current_ = end_;
                window_size_ = 0;
                return *this;
            }
            
            // Replace oldest element with the new element
            window_[oldest_idx_] = *(current_ + WindowSize - 1);
            // Update oldest index
            oldest_idx_ = (oldest_idx_ + 1) % WindowSize;
            
            return *this;
        }
        
        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        
        bool operator==(const iterator& rhs) const {
            return current_ == rhs.current_;
        }
        
    private:
        base_iterator current_;
        base_iterator end_;
        std::array<value_type, WindowSize> window_{};
        std::size_t window_size_ = 0;
        std::size_t oldest_idx_ = 0; // Track the position of the oldest element
    };
    
    sliding_median_view_array() = default;
    
    constexpr sliding_median_view_array(InputView input_range)
        : input_range_(input_range) {}
    
    constexpr iterator begin() const {
        return iterator {
            std::ranges::begin(input_range_), 
            std::ranges::end(input_range_)
        };
    }
    
    constexpr iterator end() const {
        return iterator {
            std::ranges::end(input_range_),
            std::ranges::end(input_range_)
        };
    }
};

// Deduction guide for sliding_median_view_array
template <typename R, std::size_t WindowSize>
sliding_median_view_array(R&&) -> sliding_median_view_array<std::views::all_t<R>, WindowSize>;

namespace views {
    template <std::size_t WindowSize>
    struct sliding_median_array_fn : public std::ranges::range_adaptor_closure<sliding_median_array_fn<WindowSize>> {
        template <std::ranges::viewable_range R>
        constexpr auto operator()(R&& r) const {
            return sliding_median_view_array<std::views::all_t<R>, WindowSize>{
                std::views::all(std::forward<R>(r))
            };
        }
    };
    
    template <std::size_t WindowSize>
    constexpr auto sliding_median_array() {
        return sliding_median_array_fn<WindowSize>{};
    }
}

// Benchmark functions for each implementation
double benchmark_deque(const std::vector<int>& data, std::size_t window_size) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<int> result;
    result.reserve(data.size() - window_size + 1);
    
    for (const auto& median : data | views::sliding_median_deque(window_size)) {
        result.push_back(median);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

double benchmark_slide(const std::vector<int>& data, std::size_t window_size) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<int> result;
    result.reserve(data.size() - window_size + 1);
    
    for (const auto& window_data : data | std::views::slide(window_size)) {
        std::vector<int> slide_data(window_data.begin(), window_data.end());
        std::ranges::nth_element(slide_data, slide_data.begin() + window_size/2);
        result.push_back(slide_data[window_size/2]);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

// Helper for array benchmark since window size is a template parameter
template <std::size_t WindowSize>
double benchmark_array_helper(const std::vector<int>& data) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<int> result;
    result.reserve(data.size() - WindowSize + 1);
    
    for (const auto& median : data | views::sliding_median_array<WindowSize>()) {
        result.push_back(median);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

// Handle different window sizes with a switch
double benchmark_array(const std::vector<int>& data, std::size_t window_size) {
    switch (window_size) {
        case 11: return benchmark_array_helper<11>(data);
        case 51: return benchmark_array_helper<51>(data);
        case 101: return benchmark_array_helper<101>(data);
        default:
            std::cerr << "Unsupported window size for array benchmark: " << window_size << std::endl;
            return -1.0;
    }
}

// Generate random data
std::vector<int> generate_data(std::size_t size, int min_val = -1000, int max_val = 1000) {
    std::vector<int> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(min_val, max_val);
    
    for (auto& value : data) {
        value = distrib(gen);
    }
    
    return data;
}

// Run benchmarks for multiple data sizes and window sizes
void run_benchmarks() {
    const std::vector<std::size_t> data_sizes = {30000, 100000, 300000};
    const std::vector<std::size_t> window_sizes = {11, 51, 101};
    
    // Print CSV header
    std::cout << "Data size,Window size,Deque time (s),Slide time (s),Array time (s)" << std::endl;
    
    for (auto data_size : data_sizes) {
        for (auto window_size : window_sizes) {
            // Skip window sizes that are too large for the data
            //if (window_size >= data_size) continue;
            
            std::cout << "Benchmarking data_size=" << data_size 
                      << ", window_size=" << window_size << "..." << std::flush;
            
            // Generate data once for all three benchmarks
            auto data = generate_data(data_size);
            
            // Run all three implementations
            double deque_time = benchmark_deque(data, window_size);
            double slide_time = benchmark_slide(data, window_size);
            double array_time = benchmark_array(data, window_size);
            
            // Output in CSV format with fixed precision
            std::cout << std::fixed << std::setprecision(6)
                      << "\n" << data_size << "," << window_size << "," 
                      << deque_time << "," << slide_time << "," 
                      << array_time << std::endl;
        }
    }
}

int main() {
    std::cout << "Starting benchmarks..." << std::endl;
    run_benchmarks();
    return 0;
}
