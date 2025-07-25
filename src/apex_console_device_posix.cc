// apex_console_device_posix.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include "apex_console_device.hh"

ApexConsoleDeviceSP ApexConsoleDevice::create()
{
  auto p = new ApexConsoleDevice();
  return ApexConsoleDeviceSP(p);
}

ApexConsoleDevice::ApexConsoleDevice():
  m_prev_out_was_cr(false)
{
}

bool ApexConsoleDevice::input_byte_available([[maybe_unused]] CPU6502Registers& registers)
{
  return false;
  // XXX to implement this, we have to switch from
  // C++ io streams to Posix FDs, and use select() or poll().
}

bool ApexConsoleDevice::input_byte(CPU6502Registers& registers)
{
  char c;
  std::cin.read(& c, 1);
  if (c == '\n')
  {
    c = '\r';
  }
  registers.a = c;
  return true;
}

bool ApexConsoleDevice::output_byte(CPU6502Registers& registers)
{
  char c = registers.a;
  if (m_prev_out_was_cr)
  {
    if (c == '\n')
    {
      return true;
    }
    else if (c != '\r')
    {
      m_prev_out_was_cr = false;
    }
  }
  else
  {
    if (c == '\r')
    {
      c = '\n';
      m_prev_out_was_cr = true;
    }
  }
  std::cout << c;
  return true;
}
