// apex_file_byte_device.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_FILE_BYTE_DEVICE_HH
#define APEX_FILE_BYTE_DEVICE_HH

#include <filesystem>
#include <fstream>
#include <memory>

#include "apex.hh"
#include "cpu6502.hh"

class ApexFileByteDevice: public ApexCharacterDevice
{
public:
  static std::shared_ptr<ApexFileByteDevice> create();

  void open_input_file(const std::filesystem::path& input_filename,
		       bool binary_mode = false);
  void open_output_file(const std::filesystem::path& output_filename,
			bool binary_mode = false);

  bool open_for_input(CPU6502Registers& registers) override;
  bool open_for_output(CPU6502Registers& registers) override;
  bool input_byte(CPU6502Registers& registers) override;
  bool output_byte(CPU6502Registers& registers) override;
  bool close(CPU6502Registers& registers) override;

protected:
  ApexFileByteDevice();

  bool m_input_open;
  bool m_input_binary_mode;
  bool m_input_prev_cr;
  bool m_input_at_eof;
  std::ifstream m_input_file;

  bool m_output_open;
  bool m_output_binary_mode;
  std::ofstream m_output_file;
};
using ApexFileByteDeviceSP = std::shared_ptr<ApexFileByteDevice>;

#endif // APEX_FILE_BYTE_DEVICE_HH
