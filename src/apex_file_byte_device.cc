// apex_file_byte_device.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <stdexcept>

#include "apex_file_byte_device.hh"

std::shared_ptr<ApexFileByteDevice> ApexFileByteDevice::create()
{
  auto p = new ApexFileByteDevice();
  return std::shared_ptr<ApexFileByteDevice>(p);
}

ApexFileByteDevice::ApexFileByteDevice():
  m_input_open(false),
  m_input_binary_mode(false),
  m_input_at_eof(false),
  m_output_open(false),
  m_output_binary_mode(false)
{
}

void ApexFileByteDevice::open_input_file(const std::filesystem::path& input_filename,
					 bool binary_mode)
{
  m_input_binary_mode = binary_mode;
  m_input_file.open(input_filename,
		    binary_mode ? (std::ios::in | std::ios::binary) : std::ios::in);
  if (! m_input_file.is_open())
  {
    throw std::runtime_error(std::format("couldn't open input file \"{}\"", input_filename.string()));
  }
}

void ApexFileByteDevice::open_output_file(const std::filesystem::path& output_filename,
					  bool binary_mode)
{
  m_output_binary_mode = binary_mode;
  m_output_file.open(output_filename,
		     binary_mode ? (std::ios::out | std::ios::binary) : std::ios::out);
  if (! m_output_file.is_open())
  {
    throw std::runtime_error(std::format("couldn't open output file \"{}\"", output_filename.string()));
  }
}

bool ApexFileByteDevice::open_for_input([[maybe_unused]] CPU6502Registers& registers)
{
  m_input_file.seekg(0);  // rewind
  m_input_open = true;
  m_input_at_eof = false;
  return true;
}

bool ApexFileByteDevice::open_for_output([[maybe_unused]] CPU6502Registers& registers)
{
  m_output_open = true;
  return true;
}

bool ApexFileByteDevice::input_byte(CPU6502Registers& registers)
{
  if (! m_input_open)
  {
    return false;
  }
  if (m_input_at_eof)
  {
    registers.a = 0x1a;
    return true;
  }
  std::uint8_t c;
  m_input_file.read(reinterpret_cast<char*>(& c), 1);
  if (m_input_file.eof())
  {
    registers.a = 0x1a;
    return true;
  }
  if (! m_input_binary_mode)
  {
    if (c == '\n')
    {
      c = '\r';
    }
  }
  registers.a = c;
  return true;
}

bool ApexFileByteDevice::output_byte(CPU6502Registers& registers)
{
  if (! m_output_open)
  {
    return false;
  }
  std::uint8_t c = registers.a;
  if (! m_output_binary_mode)
  {
    if (c == '\r')
    {
      c = '\n';
    }
  }
  m_output_file.write(reinterpret_cast<char*>(& c), 1);
  return true;
}

bool ApexFileByteDevice::close([[maybe_unused]] CPU6502Registers& registers)
{
  m_input_open = false;
  m_output_open = false;
  return true;
}
