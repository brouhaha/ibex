// memory.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEMORY_HH
#define MEMORY_HH

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

class Memory
{
public:
  using Address = std::uint32_t;
  using Data = std::uint8_t;
  static constexpr std::size_t APEX_PAGE_SIZE = 0x100;

  // typical size (1ull<<16) for 8-bit CPU, (1ull<<24) for 65C816
  static std::shared_ptr<Memory> create(std::size_t size);
  
  void load_raw_bin(const std::filesystem::path& object_filename,
		    Address load_address = 0x0000);

  void load_apex_bin(const std::filesystem::path& object_filename);
  void load_apex_sav(const std::filesystem::path& object_filename);

  void set_trace(bool value);

  Data read_8(Address addr);
  std::uint16_t read_16_le(Address addr);

  void write_8(Address addr, Data data);
  void write_16_le(Address addr, std::uint16_t data);

protected:
  Memory(std::size_t size);

  std::vector<std::uint8_t> m_memory;
  bool m_trace;
};

using MemorySP = std::shared_ptr<Memory>;

#endif // MEMORY_HH
