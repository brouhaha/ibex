// apex_console_device.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_CONSOLE_DEFICE_HH
#define APEX_CONSOLE_DEVICE_HH

#include <memory>

#include "apex.hh"
#include "cpu6502.hh"

class ApexConsoleDevice: public ApexCharacterDevice
{
public:
  static std::shared_ptr<ApexConsoleDevice> create();
  bool open_for_input(CPU6502Registers& registers) override;
  bool open_for_output(CPU6502Registers& registers) override;
  bool input_byte(CPU6502Registers& registers) override;
  bool output_byte(CPU6502Registers& registers) override;
  bool close(CPU6502Registers& registers) override;
protected:
  ApexConsoleDevice();
};
using ApexConsoleDeviceSP = std::shared_ptr<ApexConsoleDevice>;

#endif // APEX_CONSOLE_DEVICE_HH
