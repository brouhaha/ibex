// apex.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <format>
#include <iostream>
#include <stdexcept>

#include "apex.hh"


ApexCharacterDevice::ApexCharacterDevice()
{
}

bool ApexCharacterDevice::open_for_input([[maybe_unused]] CPU6502Registers& registers)
{
  return true;
}

bool ApexCharacterDevice::open_for_output([[maybe_unused]] CPU6502Registers& registers)
{
  return true;
}

bool ApexCharacterDevice::input_byte_available([[maybe_unused]] CPU6502Registers& registers)
{
  return false;
}

bool ApexCharacterDevice::close([[maybe_unused]] CPU6502Registers& registers)
{
  return true;
}

ApexNullDeviceSP ApexNullDevice::create()
{
  auto p = new ApexNullDevice();
  return ApexNullDeviceSP(p);
}

ApexNullDevice::ApexNullDevice()
{
}

bool ApexNullDevice::input_byte(CPU6502Registers& registers)
{
  registers.a = Apex::EOF_CHARACTER;
  return true;
}

bool ApexNullDevice::output_byte([[maybe_unused]] CPU6502Registers& registers)
{
  // discard
  return true;
}

std::shared_ptr<Apex> Apex::create(MemorySP memory_sp)
{
  auto p = new Apex(memory_sp);
  return std::shared_ptr<Apex>(p);
}

Apex::Apex(MemorySP memory_sp):
  m_memory_sp(memory_sp)
{
}

void Apex::init()
{
  m_memory_sp->write_8(SYS_PAGE_ADDRESS + SysPageOffsets::LINIDX, 0xff);
  // for unknown reasons, I2L uses the console device handler LINPTR,
  // but calls it LINIDX
  m_memory_sp->write_8(SYS_PAGE_ADDRESS + SysPageOffsets::LINPTR, 0xff);
}

void Apex::install_character_device(unsigned device_number, ApexCharacterDeviceSP device)
{
  if (device_number >= MAX_CHAR_DEVICE)
  {
    throw std::runtime_error(std::format("invalid character device number {}", device_number));
  }
  m_character_devices[device_number] = device;
}

bool Apex::vector_exec(CPU6502Registers& registers)
{
  switch (registers.pc - SYS_PAGE_ADDRESS)
  {
  case KRENTR: return krentr(registers); break;
  case KSAVER: return ksaver(registers); break;
  case KRELOD: return krelod(registers); break;
  case KHAND:  return khand (registers); break;
  case KSCAN:  return kscan (registers); break;
  case KRESTD: return krestd(registers); break;
  case KREAD:  return kread (registers); break;
  case KWRITE: return kwrite(registers); break;
  }
  std::cerr << std::format("unrecognized APEX entry vector {:04x}\n",
			     registers.pc);
  return true;
}

bool Apex::krentr([[maybe_unused]] CPU6502Registers& registers)
{
  std::cerr << "program exited via KRENTR\n";
  return true;
}

bool Apex::ksaver([[maybe_unused]] CPU6502Registers& registers)
{
  std::cerr << "program exited via KSAVER\n";
  return true;
}

bool Apex::krelod([[maybe_unused]] CPU6502Registers& registers)
{
  std::cerr << "program exited via KRELOD\n";
  return true;
}

bool Apex::khand([[maybe_unused]] CPU6502Registers& registers)
{
  // uses device specified by NOWDEV
  // function code (devicne handler entry offset) in X
  // arguments, if any, in A & Y

  std::uint8_t function = registers.x;
  std::uint8_t nowdev = m_memory_sp->read_8(SYS_PAGE_ADDRESS + SysPageOffsets::NOWDEV);

  if (m_character_devices[nowdev])
  {
  switch (function)
    {
    case 0x00:
      registers.set(CPU6502Registers::Flag::C, ! m_character_devices[nowdev]->open_for_input(registers));
      return false;
    case 0x03:
      registers.set(CPU6502Registers::Flag::C, ! m_character_devices[nowdev]->open_for_output(registers));
      return false;
    case 0x06:
      registers.set(CPU6502Registers::Flag::C, ! m_character_devices[nowdev]->input_byte(registers));
      return false;
    case 0x09:
      registers.set(CPU6502Registers::Flag::C, ! m_character_devices[nowdev]->output_byte(registers));
      return false;
    case 0x0c:
      registers.set(CPU6502Registers::Flag::C, ! m_character_devices[nowdev]->close(registers));
      return false;
    case 0x0f:
      if (nowdev > 1)
      {
	break;
      }
      registers.set(CPU6502Registers::Flag::C, ! m_character_devices[nowdev]->input_byte_available(registers));
      return false;
    }
  }
  std::cerr << std::format("bad KHAND call, NOWDEV {:02x}, A {:02x}, X {:02x}, Y {:02x}",
			   nowdev,
			   registers.a,
			   registers.x,
			   registers.y);
  return true;
}

bool Apex::kscan([[maybe_unused]] CPU6502Registers& registers)
{
  // takes pointer to a file name in address (A, Y)
  // must be 11 characters long filled with blanks
  // (8 name, 3 ext)
  // returns carry clear for success, set for failure
  // on success, fills in BLKNO, ENDBLK
  std::cerr << "KSCAN not implemented\n";
  return true;
}

bool Apex::krestd([[maybe_unused]] CPU6502Registers& registers)
{
  std::cerr << "KRESTD called, does nothing.\n";
  registers.set_c(false);
  return false;
}

bool Apex::kread([[maybe_unused]] CPU6502Registers& registers)
{
  // returns carry clear for success, set for failure
  std::cerr << "KREAD not implemented\n";
  return true;
}

bool Apex::kwrite([[maybe_unused]] CPU6502Registers& registers)
{
  // returns carry clear for success, set for failure
  std::cerr << "KWRITE not implemented\n";
  return true;
}
