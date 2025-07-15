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

ApexConsoleDevice::ApexConsoleDevice()
{
}

bool ApexConsoleDevice::open_for_input([[maybe_unused]] CPU6502Registers& registers)
{
  return true;
}

bool ApexConsoleDevice::open_for_output([[maybe_unused]] CPU6502Registers& registers)
{
  return true;
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
  if (c == '\r')
  {
    c = '\n';
  }
  std::cout << c;
  return true;
}

bool ApexConsoleDevice::close([[maybe_unused]] CPU6502Registers& registers)
{
  return true;
}
