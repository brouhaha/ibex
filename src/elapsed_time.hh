// instruction_set.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ELAPSED_TIME_HH
#define ELAPSED_TIME_HH

#include <chrono>

class ElapsedTime
{
public:
  using Clock = std::chrono::steady_clock;

  ElapsedTime();
  ~ElapsedTime();

  void start();
  void stop();

  double get_elapsed_time_seconds();

private:
  enum class State
  {
    INITIAL,
    RUNNING,
    STOPPED,
  };
  State m_state;
  Clock::time_point m_start;
  Clock::time_point m_end;
  Clock::duration m_duration;
};

#endif // ELAPSED_TIME_HH
