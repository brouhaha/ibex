// apex_printer_device.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_PRINTER_DEVICE_HH
#define APEX_PRINTER_DEVICE_HH

#include <filesystem>
#include <fstream>
#include <memory>

#include "apex.hh"
#include "cpu6502.hh"

class ApexPrinterDevice: public ApexCharacterDevice
{
public:
  static std::shared_ptr<ApexPrinterDevice> create();

  void open_output_file(const std::filesystem::path& output_filename);

  bool open_for_input(CPU6502Registers& registers) override;
  bool open_for_output(CPU6502Registers& registers) override;
  bool input_byte(CPU6502Registers& registers) override;
  bool output_byte(CPU6502Registers& registers) override;
  bool close(CPU6502Registers& registers) override;

protected:
  ApexPrinterDevice();

  bool m_output_open;
  std::ofstream m_output_file;
  bool m_prev_out_was_cr;
};
using ApexPrinterDeviceSP = std::shared_ptr<ApexPrinterDevice>;

#endif // APEX_PRINTER_DEVICE_HH
