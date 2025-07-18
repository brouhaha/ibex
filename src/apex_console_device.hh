// apex_console_device.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_CONSOLE_DEVICE_HH
#define APEX_CONSOLE_DEVICE_HH

#include <memory>

#include "apex.hh"
#include "cpu6502.hh"

class ApexConsoleDevice: public ApexCharacterDevice
{
public:
  static std::shared_ptr<ApexConsoleDevice> create();
  bool input_byte_available(CPU6502Registers& registers) override;
  bool input_byte(CPU6502Registers& registers) override;
  bool output_byte(CPU6502Registers& registers) override;

protected:
  ApexConsoleDevice();

  bool m_prev_out_was_cr;
};
using ApexConsoleDeviceSP = std::shared_ptr<ApexConsoleDevice>;

#endif // APEX_CONSOLE_DEVICE_HH
