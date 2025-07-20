// instruction_set.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef INSTRUCTION_SET_HH
#define INSTRUCTION_SET_HH

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>

class InstructionSet
{
public:
  enum class Set
  {
    UNDEFINED,
    BASE,
    ROCKWELL,  // use with BASE - present on some Rockwell NMOS microcontrollers, e.g. R6500/13, R6511Q
    CMOS,      // use with BASE
    WDC_CMOS,  // use with BASE | CMOS | ROCKWELL
    WDC_16BIT, // use with BASE | CMOS
    CBM_CMOS,  // use with BASE | CMOS | ROCKWELL
  };
  using Sets = magic_enum::containers::bitset<Set>;

  static constexpr Sets CPU_6502      = Sets({ Set::BASE });
  static constexpr Sets CPU_R6502     = Sets({ Set::BASE,            Set::ROCKWELL });
  static constexpr Sets CPU_65C02     = Sets({ Set::BASE, Set::CMOS });
  static constexpr Sets CPU_R65C02    = Sets({ Set::BASE, Set::CMOS, Set::ROCKWELL });
  static constexpr Sets CPU_WDC65C02  = Sets({ Set::BASE, Set::CMOS, Set::ROCKWELL, Set::WDC_CMOS });
  static constexpr Sets CPU_WDC65C816 = Sets({ Set::BASE, Set::CMOS,                Set::WDC_CMOS, Set::WDC_16BIT });
  static constexpr Sets CPU_65CE02    = Sets({ Set::BASE, Set::CMOS, Set::ROCKWELL,                                Set::CBM_CMOS });

  static std::shared_ptr<InstructionSet> create(const Sets& sets = { Set::BASE });

  class UnrecognizedMnemonic: public std::runtime_error
  {
  public:
    UnrecognizedMnemonic(const std::string& mnemonic);
  };

  enum class Inst
  {
    ADC,  AND,  ASL,  ASR,  ASW,  AUG,
    BBR0, BBR1, BBR2, BBR3, BBR4, BBR5, BBR6, BBR7,
    BBS0, BBS1, BBS2, BBS3, BBS4, BBS5, BBS6, BBS7,
    BCC,  BCS,  BEQ,  BIT,  BMI,  BNE,  BPL,  BRA,
    BRK,  BSR,  BVC,  BVS,  CLC,  CLD,  CLE,  CLI,
    CLV,  CMP,  CPX,  CPY,  CPZ,  DEC,  DEW,  DEX,
    DEY,  DEZ,  EOR,  INC,  INW,  INX,  INY,  INZ,
    JMP,  JSR,  LDA,  LDX,  LDY,  LDZ,  LSR,  NEG,
    NOP,  ORA,  PHA,  PHP,  PHW,  PHX,  PHY,  PHZ,
    PLA,  PLP,  PLW,  PLX,  PLY,  PLZ,
    RMB0, RMB1, RMB2, RMB3, RMB4, RMB5, RMB6, RMB7,
    ROL,  ROR,  ROW,  RTI,  RTN,  RTS,  SBC,  SEC,
    SED,  SEE,  SEI,
    SMB0, SMB1, SMB2, SMB3, SMB4, SMB5, SMB6, SMB7,
    STA,  STP,  STX,  STY,  STZ,  TAB,  TAX,  TAY,
    TAZ,  TBA,  TRB,  TSB,  TSX,  TSY,  TXA,  TXS,
    TYA,  TYS,  TZA,  WAI,
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
    ZP_IND,         // CMOS
    ZP_X_IND,
    ZP_IND_Y,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    ABSOLUTE_IND,
    ABS_X_IND,      // CMOS
    RELATIVE,
    ZP_RELATIVE,    // Rockwell BBR, BBS
    RELATIVE_16,    // Commodore 65CE02
    ST_VEC_IND_Y,   // Commodore 65CE02
  };

  struct Info
  {
    const std::string mnemonic;
    Set set;
    Inst inst;
    Mode mode;
    std::uint8_t opcode;
    std::uint8_t base_cycle_count;  // plus addrssing mode, page crossing, branch taken
    bool page_crossing_extra_cycle: 1;
    bool cmos_extra_cycle:          1; // 65C02 adds cycle to fix JMP (ABS) bug
  };

  bool valid_mnemonic(const std::string& mnemonic) const;

  const std::vector<Info>& get(const std::string& mnemonic) const;
  const Info* get(std::uint8_t opcode) const;

  std::string disassemble(std::uint16_t pc, std::array<std::uint8_t, 3> inst_bytes);

  static std::uint8_t operand_size_bytes(Mode mode);
  static std::uint8_t address_mode_added_cycles(Mode mode);

  static bool pal65_compatible_modes(Mode m1, Mode m2);

  std::ostream& print_opcode_matrix(std::ostream& os, bool detail);
  std::ostream& print_summary_table(std::ostream& os);

protected:
  InstructionSet(const Sets& sets = { Set::BASE });

  Sets m_sets;
  std::array<const Info*, 0x100> m_by_opcode;
  std::map<std::string, std::vector<Info>> m_by_mnemonic;
};

using InstructionSetSP = std::shared_ptr<InstructionSet>;

#endif // INSTRUCTION_SET_HH
