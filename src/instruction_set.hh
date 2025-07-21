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
    ROCKWELL_BIT, // use with BASE - present on some Rockwell NMOS microcontrollers, e.g. R6500/13, R6511Q
    CMOS,         // use with BASE
    WDC_CMOS,     // use with BASE | CMOS | ROCKWELL_BIT
    WDC_16_BIT,   // use with BASE | CMOS
    CBM_65CE02,   // use with BASE | CMOS | ROCKWELL_BIT
  };
  using Sets = magic_enum::containers::bitset<Set>;

  static constexpr Sets CPU_6502      = Sets({ Set::BASE });
  static constexpr Sets CPU_R6502     = Sets({ Set::BASE,            Set::ROCKWELL_BIT });
  static constexpr Sets CPU_65C02     = Sets({ Set::BASE, Set::CMOS });
  static constexpr Sets CPU_R65C02    = Sets({ Set::BASE, Set::CMOS, Set::ROCKWELL_BIT });
  static constexpr Sets CPU_WDC65C02  = Sets({ Set::BASE, Set::CMOS, Set::ROCKWELL_BIT, Set::WDC_CMOS });
  static constexpr Sets CPU_WDC65C816 = Sets({ Set::BASE, Set::CMOS,                    Set::WDC_CMOS, Set::WDC_16_BIT });
  static constexpr Sets CPU_65CE02    = Sets({ Set::BASE, Set::CMOS, Set::ROCKWELL_BIT,                                 Set::CBM_65CE02 });

  static std::shared_ptr<InstructionSet> create(const Sets& sets = { Set::BASE });

  class UnrecognizedMnemonic: public std::runtime_error
  {
  public:
    UnrecognizedMnemonic(const std::string& mnemonic);
  };

  enum class Inst
  {
    /* 000-009 */ ADC,  AND,  ASL,  ASR,  ASW,  AUG,  BBR,  BBS,  BCC,  BCS,
    /* 010-019 */ BEQ,  BIT,  BMI,  BNE,  BPL,  BRA,  BRK,  BRL,  BSR,  BVC,
    /* 020-029 */ BVS,  CLC,  CLD,  CLE,  CLI,  CLV,  CMP,  COP,  CPX,  CPY,
    /* 030-039 */ CPZ,  DEC,  DEW,  DEX,  DEY,  DEZ,  EOR,  INC,  INW,  INX,
    /* 040-049 */ INY,  INZ,  JML,  JMP,  JSL,  JSR,  LDA,  LDX,  LDY,  LDZ,
    /* 050-059 */ LSR,  MVN,  MVP,  NEG,  NOP,  ORA,  PEA,  PEI,  PER,  PHA,
    /* 060-069 */ PHB,  PHD,  PHK,  PHP,  PHW,  PHX,  PHY,  PHZ,  PLA,  PLB,
    /* 070-079 */ PLD,  PLP,  PLW,  PLX,  PLY,  PLZ,  REP,  RMB,  ROL,  ROR,
    /* 080-089 */ ROW,  RTI,  RTL,  RTN,  RTS,  SBC,  SEC,  SED,  SEE,  SEI,
    /* 090-099 */ SEP,  SMB,  STA,  STP,  STX,  STY,  STZ,  TAB,  TAX,  TAY,
    /* 100-109 */ TAZ,  TBA,  TCD,  TCS,  TDC,  TRB,  TSB,  TSX,  TSY,  TXA,
    /* 110-118 */ TXS,  TYA,  TYS,  TYX,  TZA,  WAI,  WDM,  XBA,  XCE,
  };

  enum class Mode
  {
    // real modes
    IMPLIED,        // includes WDC's "stack" mode
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
    ZP_IND_LONG,    // WDC 65C816
    ZP_IND_LONG_Y,  // WDC 65C816
    ABS_LONG,       // WDC 65C816
    ABS_LONG_X,     // WDC 65C816
    ST_REL,         // WDC 65C816
    ST_REL_IND_Y,   // WDC 65C816
    RELATIVE_16,    // WDC 65C816, Commodore 65CE02
    ST_VEC_IND_Y,   // Commodore 65CE02 - XXX is this same as WDC ST_REL_IND_Y?
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
    bool nmos_extra_cycle_forced:   1; // NMOS always takes the extra cycle on
                                       // indexed RMW instructions, whether there's
                                       // a page crossing or not
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
