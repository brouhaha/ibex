// memory.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <algorithm>
#include <format>
#include <iostream>

#include <fstream>
#include <string>

#include "apex.hh"
#include "memory.hh"
#include "utility.hh"

std::shared_ptr<Memory> Memory::create(std::size_t size)
{
  auto p = new Memory(size);
  return std::shared_ptr<Memory>(p);
}

Memory::Memory(std::size_t size):
  m_memory(size),
  m_trace(false)
{
}

void Memory::set_trace(bool value)
{
  m_trace = value;
}

static const std::string hex_digits = "0123456789abcdef";

static constexpr unsigned ADDRESS_HEX_DIGITS = 4;
static constexpr unsigned DATA_HEX_DIGITS = 2;

void Memory::load_raw_bin(const std::filesystem::path& object_filename,
			  Address load_address)
{
  std::ifstream object_file(object_filename,
			    std::ios_base::in | std::ios_base::binary);
  if (! object_file.is_open())
  {
    throw std::runtime_error("can't open object file");
  }
  char c;
  std::size_t length = 0;
  while (true)
  {
    c = object_file.get();
    if (object_file.eof())
    {
      break;
    }
    if (object_file.fail())
    {
      throw std::runtime_error("error reading object file");
    }
    std::uint8_t uc = static_cast<std::uint8_t>(c);
    m_memory.at(load_address++) = uc;
    length++;
  }
  
  std::cerr << std::format("loaded {} (0x{:04x}) bytes\n", length, length);
}

void Memory::load_apex_bin(const std::filesystem::path& object_filename)
{
  std::ifstream object_file(object_filename);
  if (! object_file.is_open())
  {
    throw std::runtime_error("can't open Apex bin file");
  }
  bool have_address = false;
  bool reading_address = false;
  Address address;
  Address value = 0;
  unsigned digit_count = 0;
  char c;
  while (true)
  {
    c = object_file.get();
    if (object_file.eof())
    {
      break;
    }
    if (object_file.fail())
    {
      throw std::runtime_error("error reading object file");
    }
    c = utility::downcase_character(c);
    if (c == '*')
    {
      reading_address = true;
      continue;
    }
    std::string::size_type v = hex_digits.find(c);
    if (v == std::string::npos)
    {
      continue;
    }
    value = (value << 4) + v;
    ++digit_count;
    if (reading_address)
    {
      if (digit_count < ADDRESS_HEX_DIGITS)
      {
	continue;
      }
      address = value;
      have_address = true;
      reading_address = false;
      digit_count = 0;
      value = 0;
      continue;
    }
    else
    {
      if (digit_count < DATA_HEX_DIGITS)
      {
	continue;
      }
      if (! have_address)
      {
	throw std::runtime_error("object file doesn't start with address");
      }
      m_memory.at(address++) = value;
      digit_count = 0;
      value = 0;
    }
  }
}

void Memory::load_apex_sav(const std::filesystem::path& save_filename)
{
  std::ifstream save_file(save_filename);
  if (! save_file.is_open())
  {
    throw std::runtime_error("can't open Apex SAV file");
  }
  std::size_t loaded_size = 0;
  Address address = 0;
  std::array<std::uint8_t, Apex::PAGE_SIZE> page;
  bool first_page = true;
  while (true)
  {
    save_file.read(reinterpret_cast<char*>(page.data()), Apex::PAGE_SIZE);
    if (save_file.eof())
    {
      break;
    }
    if (save_file.fail())
    {
      throw std::runtime_error("error reading SAV file");
    }
    if (first_page)
    {
      // copy first part of first page of SAV file to program area in SYSPAG
      std::copy(page.begin(), page.begin() + Apex::SYS_PAGE_PROGRAM_AREA_SIZE, m_memory.begin() + Apex::SYS_PAGE_ADDRESS);
      // copy remainder of first page to zero page
      std::copy(page.begin() + Apex::SYS_PAGE_PROGRAM_AREA_SIZE, page.end(), m_memory.begin() + Apex::SYS_PAGE_PROGRAM_AREA_SIZE);
      address = read_16_le(Apex::SYS_PAGE_ADDRESS + Apex::SysPageOffsets::USRMEM);
      std::cerr << std::format("loading at {:04x}\n", address);
      first_page = false;
    }
    else
    {
      std::copy(page.begin(), page.end(), m_memory.begin() + address);
      address += Apex::PAGE_SIZE;
      loaded_size += Apex::PAGE_SIZE;
    }
  }
  std::cerr << std::format("loading ended at {:04x}, size {}\n", address - 1, loaded_size);
}

void Memory::dump_raw_bin(const std::filesystem::path& raw_bin_filename,
			  Address start_address,
			  std::size_t size)
{
  if (size == 0)
  {
    size = m_memory.size() - start_address;
  }
  std::ofstream dump_file(raw_bin_filename,
			  std::ios_base::out | std::ios_base::binary);
  if (! dump_file.is_open())
  {
    throw std::runtime_error("can't open memory dump file");
  }
  dump_file.write(reinterpret_cast<char*>(m_memory.data() + start_address),
		  size);
}

Memory::Data Memory::read_8(Address addr)
{
  return m_memory.at(addr);
}

std::uint16_t Memory::read_16_le(Address addr)
{
  return m_memory.at(addr) | (m_memory.at(addr + 1) << 8);
}

void Memory::write_8(Address addr, Data data)
{
  if (m_trace)
  {
    std::cout << std::format("    wrote addr {:04x} data {:02x}\n", addr, data);
  }
  m_memory.at(addr) = data;
}

void Memory::write_16_le(Address addr, std::uint16_t data)
{
  m_memory.at(addr)     = data & 0xff;
  m_memory.at(addr + 1) = data >> 8;
}
