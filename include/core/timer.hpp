#ifndef FLUID_SIMULATOR_TIMER_HPP
#define FLUID_SIMULATOR_TIMER_HPP

#include <chrono>

namespace fluid::core {

class Timer {
  public:
    Timer() = default;

    void update();

    float getDeltaTime() const noexcept;

  private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point _last = Clock::now();
    float _delta_time = 0.0f;
};

} // namespace fluid::core

#endif