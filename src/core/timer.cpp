#include "core/timer.hpp"

namespace fluid::core {

void Timer::update() {
    const auto current = Clock::now();

    _delta_time = std::chrono::duration<float>(current - _last).count();

    _last = current;
}

float Timer::getDeltaTime() const noexcept {
    return _delta_time;
}

} // namespace fluid::core