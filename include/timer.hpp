#pragma once

#include <chrono>
#include <print>
#include <string>

namespace mk {

struct timer {
    using clock = std::chrono::steady_clock;

    timer(std::string name) : _name{std::move(name)}, _start{clock::now()} {}
    ~timer() {
        namespace chr = std::chrono;
        using presentation_unit = chr::duration<float, std::milli>;

        auto stop = clock::now();
        auto elapsed = stop - _start;
        std::print("* {} elapsed: {}\n", _name,
                   chr::duration_cast<presentation_unit>(elapsed));
    }

   private:
    std::string _name;
    clock::time_point _start;
};

}  // namespace mk
