// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <stdexcept>

#include "elapsed_time.hh"


using namespace std::literals::chrono_literals;


ElapsedTime::ElapsedTime():
  m_state(State::INITIAL)
{
}


ElapsedTime::~ElapsedTime()
{
  stop();
}


void ElapsedTime::start()
{
  if (m_state != State::INITIAL)
  {
    throw std::logic_error("starting ElapsedTime that has already been started");
  }
  m_state = State::RUNNING;
  m_start = Clock::now();
}


void ElapsedTime::stop()
{
  if (m_state != State::RUNNING)
  {
    return;
  }
  m_state = State::STOPPED;
  m_end = Clock::now();
  m_duration = m_end - m_start;
}


double ElapsedTime::get_elapsed_time_seconds()
{
  Clock::duration elapsed_time;

  switch (m_state)
  {
  case State::INITIAL:
    elapsed_time = 0s;
    break;

  case State::RUNNING:
    {
      Clock::time_point now = Clock::now();
      elapsed_time = now - m_start;
    }
    break;

  case State::STOPPED:
    elapsed_time = m_duration;
    break;
  }

  return std::chrono::duration_cast<std::chrono::duration<double>>(elapsed_time).count();
}
