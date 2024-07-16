#ifdef USE_THIRDPARTY
#include <fmt/chrono.h>
#include <fmt/format.h>
#else
#include <format>
#include <print>
#endif

#if PARALLEL
#include <execution>
#define SEQ std::execution::seq,
#define PAR std::execution::par,
#else
#define SEQ
#define PAR
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdefaulted-function-deleted"
#pragma GCC diagnostic ignored "-Wdeprecated-this-capture"
#include <https://raw.githubusercontent.com/alice-viola/ThreadPool/master/threadpool.hpp>
#pragma GCC diagnostic pop
//
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <ranges>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef THREAD_POOL_COUNT
#define THREAD_POOL_COUNT std::thread::hardware_concurrency()
#endif

#ifdef USE_THIRDPARTY
using fmt::format;
using fmt::print;
using fmt::to_string;
#else
using std::format;
using std::print;
using std::to_string;
#endif

using key_type = std::string;
using value_type = std::tuple<int>;
using partial_result = std::unordered_map<key_type, value_type>;
using final_result = std::vector<std::tuple<key_type, value_type>>;

static inline partial_result generate_partial_results(
    char prefix, size_t n, [[maybe_unused]] double unique_percentage) {
    partial_result res;
    res.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        res.emplace(format("{}{}", prefix, i), std::make_tuple(i));
    }
    return res;
}

static inline void inplace_merge(const partial_result& pr,
                                 final_result::iterator begin,
                                 final_result::iterator end) {
    assert(size(pr) == std::distance(begin, end));
    for (const auto& v : pr) {
        assert(begin != end);
        *begin++ = v;
    }
}

#if defined(USE_THREAD_POOL) && USE_THREAD_POOL
using thread_pool = astp::ThreadPool;
#else
struct thread_pool {
    thread_pool(size_t thread_count = std::thread::hardware_concurrency()) {
        _threads.reserve(thread_count);
    }

    template <typename F>
    void push(F&& f) {
        _threads.emplace_back(std::forward<F>(f));
    }

    void wait() { std::ranges::for_each(_threads, &std::thread::join); }

   private:
    std::vector<std::thread> _threads;
};
#endif

static inline final_result merge_partial_results(
    std::vector<partial_result> partial_results) {
    final_result res;

#ifndef NDEBUG
    auto total_count = std::transform_reduce(
        PAR partial_results.cbegin(), partial_results.cend(), std::size_t{0},
        std::plus{}, std::mem_fn(&partial_result::size));
#endif

    std::vector<size_t> counts(1 + partial_results.size());
    [[maybe_unused]] auto out_it = std::transform_inclusive_scan(
        PAR partial_results.cbegin(), partial_results.cend(),
        counts.begin() + 1, std::plus{}, std::mem_fn(&partial_result::size),
        std::size_t{0});
    assert(out_it == counts.end());
    assert(total_count == counts.back());
    total_count = counts.back();

    res.resize(total_count);

    auto p = thread_pool{/*max_threads=*/THREAD_POOL_COUNT};
    size_t count_index = 0;
    for (const auto& pr : partial_results) {
        size_t start_idx = counts[count_index];
        size_t end_idx = counts[count_index + 1];
        ++count_index;

        print("merge [{}:{})\n", start_idx, end_idx);
        auto begin = res.begin();
        p.push([&pr, b = begin + start_idx, e = begin + end_idx]() {
            inplace_merge(pr, b, e);
        });
    }
    p.wait();
    return res;
}

struct timer {
    using clock = std::chrono::steady_clock;

    timer(std::string name) : _name{std::move(name)}, _start{clock::now()} {}
    ~timer() {
        namespace chr = std::chrono;
        using presentation_unit = chr::duration<float, std::milli>;

        auto stop = clock::now();
        auto elapsed = stop - _start;
        print("* {} elapsed: {}\n", _name,
              chr::duration_cast<presentation_unit>(elapsed));
    }

   private:
    std::string _name;
    clock::time_point _start;
};

int main() {
    constexpr size_t partial_count = 32;
    std::vector<partial_result> partial_results;
    partial_results.reserve(partial_count);

    {
        timer _("generate");
        for (size_t i = 0; i < partial_count; ++i) {
            partial_results.emplace_back(
                generate_partial_results('a' + i, 2'000'0, 0.10));
        }
    }

    final_result fr;
    {
        timer _("merge");
        fr = merge_partial_results(std::move(partial_results));
    }

    print("final: size = {}, [\n", std::size(fr));
    // for (const auto& v : fr) {
    //     print("'{}': {{ {} }}\n", std::get<0>(v),
    //     std::get<0>(std::get<1>(v)));
    // }
    // print("\n]");
}
