// apex_printer_device.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <stdexcept>

#include "apex_printer_device.hh"

std::shared_ptr<ApexPrinterDevice> ApexPrinterDevice::create()
{
  auto p = new ApexPrinterDevice();
  return std::shared_ptr<ApexPrinterDevice>(p);
}

ApexPrinterDevice::ApexPrinterDevice():
  m_output_open(false)
{
}

void ApexPrinterDevice::open_output_file(const std::filesystem::path& output_filename)
{
  m_output_file.open(output_filename);
  if (! m_output_file.is_open())
  {
    throw std::runtime_error(std::format("couldn't open printer file \"{}\"", output_filename.string()));
  }
}

bool ApexPrinterDevice::open_for_input([[maybe_unused]] CPU6502Registers& registers)
{
  return false;
}

bool ApexPrinterDevice::open_for_output([[maybe_unused]] CPU6502Registers& registers)
{
  m_output_open = true;
  return true;
}

bool ApexPrinterDevice::input_byte(CPU6502Registers& registers)
{
  registers.a = 0x1a;
  return false;
}

bool ApexPrinterDevice::output_byte(CPU6502Registers& registers)
{
  if (! m_output_open)
  {
    return false;
  }
  std::uint8_t c = registers.a;
  if (c == '\r')
  {
    c = '\n';
  }
  m_output_file.write(reinterpret_cast<char*>(& c), 1);
  return true;
}

bool ApexPrinterDevice::close([[maybe_unused]] CPU6502Registers& registers)
{
  m_output_open = false;
  return true;
}
