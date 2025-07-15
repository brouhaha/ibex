// instruction_set.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef INSTRUCTION_SET_HH
#define INSTRUCTION_SET_HH

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>

class InstructionSet
{
public:
  static std::shared_ptr<InstructionSet> create();

  class UnrecognizedMnemonic: public std::runtime_error
  {
  public:
    UnrecognizedMnemonic(const std::string& mnemonic);
  };

  enum class Set
  {
    UNDEFINED,
    BASE,
    ROCKWELL,
    CMOS,
  };

  enum class Inst
  {
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI,
    BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI,
    CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR,
    INC, INX, INY, JMP, JSR, LDA, LDX, LDY,
    LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL,
    ROR, RTI, RTS, SBC, SEC, SED, SEI, STA,
    STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
  };

  enum class Mode
  {
    // real modes
    IMPLIED,
    ACCUMULATOR,
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    ZP_X_IND,
    ZP_IND_Y,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    ABSOLUTE_IND,
    RELATIVE,
  };

  static std::uint32_t get_length(Mode mode);

  struct Info
  {
    const std::string mnemonic;
    Set set;
    Inst inst;
    Mode mode;
    std::uint8_t opcode;
  };

  bool valid_mnemonic(const std::string& mnemonic) const;

  const std::vector<Info>& get(const std::string& mnemonic) const;
  const Info* get(std::uint8_t opcode) const;

  std::string disassemble(std::uint16_t pc, std::array<std::uint8_t, 3> inst_bytes);

  static std::uint8_t operand_size_bytes(Mode mode);

  static bool pal65_compatible_modes(Mode m1, Mode m2);

protected:
  InstructionSet();

  std::array<const Info*, 0x100> m_by_opcode;
  std::map<std::string, std::vector<Info>> m_by_mnemonic;
};

using InstructionSetSP = std::shared_ptr<InstructionSet>;

#endif // INSTRUCTION_SET_HH
