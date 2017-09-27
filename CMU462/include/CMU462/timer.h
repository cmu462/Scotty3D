#ifndef CMU462_TIMER_H
#define CMU462_TIMER_H

#include "CMU462.h"
#include <chrono>

namespace CMU462 {

/**
 * A basic timer class.
 */
class Timer {
 public:

  /**
   * Starts the timer
   */
  inline void start() {
    t0 = std::chrono::steady_clock::now();
  }

  /**
   * Stops the timer
   */
  inline void stop() {
    t1 = std::chrono::steady_clock::now();
  }

  /**
   * Return duration between the last call to start and last call to stop
   */
  inline double duration() {
    return (std::chrono::duration<double>(t1 - t0)).count();
  }

 private:

  std::chrono::time_point<std::chrono::steady_clock> t0;
  std::chrono::time_point<std::chrono::steady_clock> t1;

};

} // namespace CMU462

#endif //CMU462_TIMER_H
