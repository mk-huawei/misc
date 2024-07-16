#include <fmt/chrono.h>
#include <fmt/format.h>

#include <cassert>
#include <chrono>
#include <utility>

namespace mk {

enum class meters : size_t;

inline namespace literals {

constexpr meters operator""_m(unsigned long long v) { return meters{v}; }

}  // namespace literals

struct wavelight {
    using time_unit =
        std::chrono::milliseconds;  // Enough precision for lights.

    constexpr time_unit compute_light_delta() const noexcept {  //
        using namespace std::literals;

        assert(light_count > 0u);
        float distance_between_lights =
            static_cast<float>(std::to_underlying(lap_distance)) / light_count;

        assert(std::to_underlying(lap_distance) > 0);
        float laps = static_cast<float>(std::to_underlying(total_distance)) /
                     std::to_underlying(lap_distance);

        using rep = time_unit::rep;
        auto lap_time = time_unit{
            static_cast<rep>(total_duration.count() / laps)};  // narrow_cast
        time_unit time_between_lights = lap_time / light_count;
        return time_between_lights;
    }

    size_t light_count{0};
    meters lap_distance{0};
    meters total_distance{0};
    time_unit total_duration{0};
};

struct wavelight_builder {
    constexpr wavelight_builder& set_light_count(size_t light_count) noexcept {
        _result.light_count = light_count;
        return *this;
    }

    constexpr wavelight_builder& set_lap_distance(
        meters lap_distance) noexcept {
        _result.lap_distance = lap_distance;
        return *this;
    }

    constexpr wavelight_builder& set_total_distance(
        meters total_distance) noexcept {
        _result.total_distance = total_distance;
        return *this;
    }

    constexpr wavelight_builder& set_total_duration(
        wavelight::time_unit total_duration) noexcept {
        _result.total_duration = total_duration;
        return *this;
    }

    constexpr wavelight result() const noexcept { return _result; }

   private:
    wavelight _result;
};

}  // namespace mk

int main() {
    using namespace std::literals;
    using namespace mk::literals;

    mk::wavelight wl1(100, 400_m, 400_m, 60s);
    fmt::print("wl1 diff: {}\n", wl1.compute_light_delta());
    // 400m in 60s, light every 400/100=4m, 4m in 0.6s
    assert(wl1.compute_light_delta() == 600ms);

    mk::wavelight wl2(100, 400_m, 800_m, 110s);
    fmt::print("wl2 diff: {}\n", wl2.compute_light_delta());
    // 800m in 110s, 400m in 55s, light every 400/100=4m, 4m in 0.55s
    assert(wl2.compute_light_delta() == 550ms);

    mk::wavelight wl3 = mk::wavelight_builder{}  //
                            .set_light_count(100)
                            .set_lap_distance(400_m)
                            .set_total_distance(1000_m)
                            .set_total_duration(3min)
                            .result()

        ;
    fmt::print("wl3 diff: {}\n", wl3.compute_light_delta());
    assert(wl3.compute_light_delta() == 720ms);
}
