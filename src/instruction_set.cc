// instruction_set.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <bitset>
#include <format>
#include <stdexcept>

#include "instruction_set.hh"
#include "utility.hh"

using enum InstructionSet::Set;
using enum InstructionSet::Inst;
using enum InstructionSet::Mode;

const magic_enum::containers::array<InstructionSet::Mode, std::uint8_t> s_operand_size_bytes
{
  /* IMPLIED      */ 0,
  /* ACCUMULATOR  */ 0,
  /* IMMEDIATE    */ 1,
  /* ZERO_PAGE    */ 1,
  /* ZERO_PAGE_X  */ 1,
  /* ZERO_PAGE_Y  */ 1,
  /* ZP_X_IND     */ 1,
  /* ZP_IND_Y     */ 1,
  /* ABSOLUTE     */ 2,
  /* ABSOLUTE_X   */ 2,
  /* ABSOLUTE_Y   */ 2,
  /* ABSOLUTE_IND */ 2,
  /* RELATIVE     */ 1,
};

const magic_enum::containers::array<InstructionSet::Mode, std::string> s_mos_address_mode_prefixes
{
  /* IMPLIED      */ "",
  /* ACCUMULATOR  */ "",
  /* IMMEDIATE    */ "#",
  /* ZERO_PAGE    */ "",
  /* ZERO_PAGE_X  */ "",
  /* ZERO_PAGE_Y  */ "",
  /* ZP_X_IND     */ "(",
  /* ZP_IND_Y     */ "(",
  /* ABSOLUTE     */ "",
  /* ABSOLUTE_X   */ "",
  /* ABSOLUTE_Y   */ "",
  /* ABSOLUTE_IND */ "(",
  /* RELATIVE     */ "",
};

const magic_enum::containers::array<InstructionSet::Mode, std::string> s_mos_address_mode_suffixes
{
  /* IMPLIED      */ "",
  /* ACCUMULATOR  */ "",
  /* IMMEDIATE    */ "",
  /* ZERO_PAGE    */ "",
  /* ZERO_PAGE_X  */ ",x",
  /* ZERO_PAGE_Y  */ ",y",
  /* ZP_X_IND     */ ",x)",
  /* ZP_IND_Y     */ "),y",
  /* ABSOLUTE     */ "",
  /* ABSOLUTE_X   */ ",x",
  /* ABSOLUTE_Y   */ ",y",
  /* ABSOLUTE_IND */ ")",
  /* RELATIVE     */ "",
};

const magic_enum::containers::array<InstructionSet::Mode, std::string> s_pal65_address_mode_suffixes
{
  /* IMPLIED      */ "",
  /* ACCUMULATOR  */ "a",
  /* IMMEDIATE    */ "#",
  /* ZERO_PAGE    */ "",
  /* ZERO_PAGE_X  */ "x",
  /* ZERO_PAGE_Y  */ "y",
  /* ZP_X_IND     */ "x@",
  /* ZP_IND_Y     */ "@y",
  /* ABSOLUTE     */ "",
  /* ABSOLUTE_X   */ "x",
  /* ABSOLUTE_Y   */ "y",
  /* ABSOLUTE_IND */ "@",
  /* RELATIVE     */ "",
};

const std::vector<InstructionSet::Info> s_main_table
{
  { "adc", BASE, ADC, IMMEDIATE,    0x69 },
  { "adc", BASE, ADC, ZERO_PAGE,    0x65 },
  { "adc", BASE, ADC, ZERO_PAGE_X,  0x75 },
  { "adc", BASE, ADC, ZP_X_IND,     0x61 },
  { "adc", BASE, ADC, ZP_IND_Y,     0x71 },
  { "adc", BASE, ADC, ABSOLUTE,     0x6d },
  { "adc", BASE, ADC, ABSOLUTE_X,   0x7d },
  { "adc", BASE, ADC, ABSOLUTE_Y,   0x79 },

  { "and", BASE, AND, IMMEDIATE,    0x29 },
  { "and", BASE, AND, ZERO_PAGE,    0x25 },
  { "and", BASE, AND, ZERO_PAGE_X,  0x35 },
  { "and", BASE, AND, ZP_X_IND,     0x21 },
  { "and", BASE, AND, ZP_IND_Y,     0x31 },
  { "and", BASE, AND, ABSOLUTE,     0x2d },
  { "and", BASE, AND, ABSOLUTE_X,   0x3d },
  { "and", BASE, AND, ABSOLUTE_Y,   0x39 },

  { "asl", BASE, ASL, ACCUMULATOR,  0x0a },
  { "asl", BASE, ASL, ZERO_PAGE,    0x06 },
  { "asl", BASE, ASL, ZERO_PAGE_X,  0x16 },
  { "asl", BASE, ASL, ABSOLUTE,     0x0e },
  { "asl", BASE, ASL, ABSOLUTE_X,   0x1e },

  { "bcc", BASE, BCC, RELATIVE,     0x90 },

  { "bcs", BASE, BCS, RELATIVE,     0xb0 },

  { "beq", BASE, BEQ, RELATIVE,     0xf0 },

  { "bit", BASE, BIT, ZERO_PAGE,    0x24 },
  { "bit", BASE, BIT, ABSOLUTE,     0x2c },

  { "bmi", BASE, BMI, RELATIVE,     0x30 },

  { "bne", BASE, BNE, RELATIVE,     0xd0 },

  { "bpl", BASE, BPL, RELATIVE,     0x10 },

  { "brk", BASE, BRK, IMPLIED,      0x00 },

  { "bvc", BASE, BVC, RELATIVE,     0x50 },

  { "bvs", BASE, BVS, RELATIVE,     0x70 },

  { "clc", BASE, CLC, IMPLIED,      0x18 },

  { "cld", BASE, CLD, IMPLIED,      0xd8 },

  { "cli", BASE, CLI, IMPLIED,      0x58 },

  { "clv", BASE, CLV, IMPLIED,      0xb8 },

  { "cmp", BASE, CMP, IMMEDIATE,    0xc9 },
  { "cmp", BASE, CMP, ZERO_PAGE,    0xc5 },
  { "cmp", BASE, CMP, ZERO_PAGE_X,  0xd5 },
  { "cmp", BASE, CMP, ZP_X_IND,     0xc1 },
  { "cmp", BASE, CMP, ZP_IND_Y,     0xd1 },
  { "cmp", BASE, CMP, ABSOLUTE,     0xcd },
  { "cmp", BASE, CMP, ABSOLUTE_X,   0xdd },
  { "cmp", BASE, CMP, ABSOLUTE_Y,   0xd9 },

  { "cpx", BASE, CPX, IMMEDIATE,    0xe0 },
  { "cpx", BASE, CPX, ZERO_PAGE,    0xe4 },
  { "cpx", BASE, CPX, ABSOLUTE,     0xec },

  { "cpy", BASE, CPY, IMMEDIATE,    0xc0 },
  { "cpy", BASE, CPY, ZERO_PAGE,    0xc4 },
  { "cpy", BASE, CPY, ABSOLUTE,     0xcc },

  { "dec", BASE, DEC, ZERO_PAGE,    0xc6 },
  { "dec", BASE, DEC, ZERO_PAGE_X,  0xd6 },
  { "dec", BASE, DEC, ABSOLUTE,     0xce },
  { "dec", BASE, DEC, ABSOLUTE_X,   0xde },

  { "dex", BASE, DEX, IMPLIED,      0xca },

  { "dey", BASE, DEY, IMPLIED,      0x88 },

  { "eor", BASE, EOR, IMMEDIATE,    0x49 },
  { "eor", BASE, EOR, ZERO_PAGE,    0x45 },
  { "eor", BASE, EOR, ZERO_PAGE_X,  0x55 },
  { "eor", BASE, EOR, ZP_X_IND,     0x41 },
  { "eor", BASE, EOR, ZP_IND_Y,     0x51 },
  { "eor", BASE, EOR, ABSOLUTE,     0x4d },
  { "eor", BASE, EOR, ABSOLUTE_X,   0x5d },
  { "eor", BASE, EOR, ABSOLUTE_Y,   0x59 },

  { "inc", BASE, INC, ZERO_PAGE,    0xe6 },
  { "inc", BASE, INC, ZERO_PAGE_X,  0xf6 },
  { "inc", BASE, INC, ABSOLUTE,     0xee },
  { "inc", BASE, INC, ABSOLUTE_X,   0xfe },

  { "inx", BASE, INX, IMPLIED,      0xe8 },

  { "iny", BASE, INY, IMPLIED,      0xc8 },

  { "jmp", BASE, JMP, ABSOLUTE,     0x4c },
  { "jmp", BASE, JMP, ABSOLUTE_IND, 0x6c },

  { "jsr", BASE, JSR, ABSOLUTE,     0x20 },
  
  { "lda", BASE, LDA, IMMEDIATE,    0xa9 },
  { "lda", BASE, LDA, ZERO_PAGE,    0xa5 },
  { "lda", BASE, LDA, ZERO_PAGE_X,  0xb5 },
  { "lda", BASE, LDA, ZP_X_IND,     0xa1 },
  { "lda", BASE, LDA, ZP_IND_Y,     0xb1 },
  { "lda", BASE, LDA, ABSOLUTE,     0xad },
  { "lda", BASE, LDA, ABSOLUTE_X,   0xbd },
  { "lda", BASE, LDA, ABSOLUTE_Y,   0xb9 },

  { "ldx", BASE, LDX, IMMEDIATE,    0xa2 },
  { "ldx", BASE, LDX, ZERO_PAGE,    0xa6 },
  { "ldx", BASE, LDX, ZERO_PAGE_Y,  0xb6 },
  { "ldx", BASE, LDX, ABSOLUTE,     0xae },
  { "ldx", BASE, LDX, ABSOLUTE_Y,   0xbe },

  { "ldy", BASE, LDY, IMMEDIATE,    0xa0 },
  { "ldy", BASE, LDY, ZERO_PAGE,    0xa4 },
  { "ldy", BASE, LDY, ZERO_PAGE_X,  0xb4 },
  { "ldy", BASE, LDY, ABSOLUTE,     0xac },
  { "ldy", BASE, LDY, ABSOLUTE_X,   0xbc },

  { "lsr", BASE, LSR, ACCUMULATOR,  0x4a },
  { "lsr", BASE, LSR, ZERO_PAGE,    0x46 },
  { "lsr", BASE, LSR, ZERO_PAGE_X,  0x56 },
  { "lsr", BASE, LSR, ABSOLUTE,     0x4e },
  { "lsr", BASE, LSR, ABSOLUTE_X,   0x5e },

  { "nop", BASE, NOP, IMPLIED,      0xea },

  { "ora", BASE, ORA, IMMEDIATE,    0x09 },
  { "ora", BASE, ORA, ZERO_PAGE,    0x05 },
  { "ora", BASE, ORA, ZERO_PAGE_X,  0x15 },
  { "ora", BASE, ORA, ZP_X_IND,     0x01 },
  { "ora", BASE, ORA, ZP_IND_Y,     0x11 },
  { "ora", BASE, ORA, ABSOLUTE,     0x0d },
  { "ora", BASE, ORA, ABSOLUTE_X,   0x1d },
  { "ora", BASE, ORA, ABSOLUTE_Y,   0x19 },

  { "pha", BASE, PHA, IMPLIED,      0x48 },
  
  { "php", BASE, PHP, IMPLIED,      0x08 },

  { "pla", BASE, PLA, IMPLIED,      0x68 },

  { "plp", BASE, PLP, IMPLIED,      0x28 },

  { "rol", BASE, ROL, ACCUMULATOR,  0x2a },
  { "rol", BASE, ROL, ZERO_PAGE,    0x26 },
  { "rol", BASE, ROL, ZERO_PAGE_X,  0x36 },
  { "rol", BASE, ROL, ABSOLUTE,     0x2e },
  { "rol", BASE, ROL, ABSOLUTE_X,   0x3e },

  // very early 6502 didn't have ROR
  { "ror", BASE, ROR, ACCUMULATOR,  0x6a },
  { "ror", BASE, ROR, ZERO_PAGE,    0x66 },
  { "ror", BASE, ROR, ZERO_PAGE_X,  0x76 },
  { "ror", BASE, ROR, ABSOLUTE,     0x6e },
  { "ror", BASE, ROR, ABSOLUTE_X,   0x7e },

  { "rti", BASE, RTI, IMPLIED,      0x40 },

  { "rts", BASE, RTS, IMPLIED,      0x60 },

  { "sbc", BASE, SBC, IMMEDIATE,    0xe9 },
  { "sbc", BASE, SBC, ZERO_PAGE,    0xe5 },
  { "sbc", BASE, SBC, ZERO_PAGE_X,  0xf5 },
  { "sbc", BASE, SBC, ZP_X_IND,     0xe1 },
  { "sbc", BASE, SBC, ZP_IND_Y,     0xf1 },
  { "sbc", BASE, SBC, ABSOLUTE,     0xed },
  { "sbc", BASE, SBC, ABSOLUTE_X,   0xfd },
  { "sbc", BASE, SBC, ABSOLUTE_Y,   0xf9 },

  { "sec", BASE, SEC, IMPLIED,      0x38 },

  { "sed", BASE, SED, IMPLIED,      0xf8 },

  { "sei", BASE, SEI, IMPLIED,      0x78 },

  { "sta", BASE, STA, ZERO_PAGE,    0x85 },
  { "sta", BASE, STA, ZERO_PAGE_X,  0x95 },
  { "sta", BASE, STA, ZP_X_IND,     0x81 },
  { "sta", BASE, STA, ZP_IND_Y,     0x91 },
  { "sta", BASE, STA, ABSOLUTE,     0x8d },
  { "sta", BASE, STA, ABSOLUTE_X,   0x9d },
  { "sta", BASE, STA, ABSOLUTE_Y,   0x99 },

  { "stx", BASE, STX, ZERO_PAGE,    0x86 },
  { "stx", BASE, STX, ZERO_PAGE_Y,  0x96 },
  { "stx", BASE, STX, ABSOLUTE,     0x8e },

  { "sty", BASE, STY, ZERO_PAGE,    0x84 },
  { "sty", BASE, STY, ZERO_PAGE_X,  0x94 },
  { "sty", BASE, STY, ABSOLUTE,     0x8c },

  { "tax", BASE, TAX, IMPLIED,      0xaa },

  { "tay", BASE, TAY, IMPLIED,      0xa8 },

  { "tsx", BASE, TSX, IMPLIED,      0xba },

  { "txa", BASE, TXA, IMPLIED,      0x8a },

  { "txs", BASE, TXS, IMPLIED,      0x9a },

  { "tya", BASE, TYA, IMPLIED,      0x98 },
};

InstructionSet::UnrecognizedMnemonic::UnrecognizedMnemonic(const std::string& mnemonic):
  std::runtime_error(std::format("unrecognized mnemonic {}", mnemonic))
{
}

std::shared_ptr<InstructionSet> InstructionSet::create()
{
  auto p = new InstructionSet();
  return std::shared_ptr<InstructionSet>(p);
}

InstructionSet::InstructionSet()
{
  for (const auto& info: s_main_table)
  {
    std::string pal65_mnemonic = info.mnemonic + s_pal65_address_mode_suffixes[info.mode];
    if (m_by_opcode[info.opcode])
    {
      throw std::logic_error(std::format("duplicate opcode {:02x}", info.opcode));
    }
    m_by_opcode[info.opcode] = &info;
    if (m_by_mnemonic.contains(pal65_mnemonic))
    {
      if (! pal65_compatible_modes(m_by_mnemonic.at(pal65_mnemonic).at(0).mode, info.mode))
      {
	throw std::logic_error(std::format("duplicate PAL65 mnemonic {}", pal65_mnemonic));
      }
    }
    m_by_mnemonic[pal65_mnemonic].push_back(info);
  }
}

bool InstructionSet::valid_mnemonic(const std::string& mnemonic) const
{
  std::string s = utility::downcase_string(mnemonic);
  return m_by_mnemonic.contains(s);
}

const std::vector<InstructionSet::Info>& InstructionSet::get(const std::string& mnemonic) const
{
  std::string s = utility::downcase_string(mnemonic);
  if (! m_by_mnemonic.contains(s))
  {
    throw UnrecognizedMnemonic(mnemonic);
  }
  return m_by_mnemonic.at(s);
}

const InstructionSet::Info* InstructionSet::get(std::uint8_t opcode) const
{
  return m_by_opcode[opcode];
}

std::uint32_t InstructionSet::get_length(Mode mode)
{
  switch (mode)
  {
  case IMPLIED:
  case ACCUMULATOR:
    return 1;

  case IMMEDIATE:
    return 2;

  case ZERO_PAGE:
  case ZERO_PAGE_X:
  case ZERO_PAGE_Y:
  case ZP_X_IND:
  case ZP_IND_Y:
    return 2;

  case ABSOLUTE:
  case ABSOLUTE_X:
  case ABSOLUTE_Y:
  case ABSOLUTE_IND:
    return 3;

  case RELATIVE:
    return 2;

  default:
    throw std::logic_error("unrecognized address mode");
  }
}

std::string InstructionSet::disassemble(std::uint16_t pc, std::array<std::uint8_t, 3> inst_bytes)
{
  std::uint8_t opcode = inst_bytes[0];
  const Info* info = get(opcode);
  if (! info)
  {
    return std::format("undefined {:02x}", opcode);
  }
  std::string s = info->mnemonic;
  std::uint16_t operand;
  switch (info->mode)
  {
  case IMPLIED:
    break;
  case ACCUMULATOR:
    s += " a";
    break;
  case IMMEDIATE:
  case ZERO_PAGE:
  case ZERO_PAGE_X:
  case ZERO_PAGE_Y:
  case ZP_X_IND:
  case ZP_IND_Y:
    operand = inst_bytes[1];
    s += std::format(" {}${:02x}{}",
		     s_mos_address_mode_prefixes[info->mode],
		     operand,
		     s_mos_address_mode_suffixes[info->mode]);
    break;
  case ABSOLUTE:
  case ABSOLUTE_X:
  case ABSOLUTE_Y:
  case ABSOLUTE_IND:
    operand = inst_bytes[1] | (inst_bytes[2] << 8);
    s += std::format(" {}${:04x}{}",
		     s_mos_address_mode_prefixes[info->mode],
		     operand,
		     s_mos_address_mode_suffixes[info->mode]);
    break;
  case RELATIVE:
    operand = inst_bytes[1];
    if (operand & 0x80)
    {
      operand |= 0xff00;
    }
    operand += pc + 2;
    s += std::format(" ${:04x}", operand);
    break;
  default:
    throw std::logic_error("unknown addressing mode");
  }
  return s;
}


std::uint8_t InstructionSet::operand_size_bytes(Mode mode)
{
  return s_operand_size_bytes[mode];
}

bool InstructionSet::pal65_compatible_modes(Mode m1, Mode m2)
{
  if (((m1 == ZERO_PAGE) && (m2 == ABSOLUTE)) ||
      ((m1 == ABSOLUTE)  && (m2 == ZERO_PAGE)))
  {
    return true;
  }
  if (((m1 == ZERO_PAGE_X) && (m2 == ABSOLUTE_X)) ||
      ((m1 == ABSOLUTE_X)  && (m2 == ZERO_PAGE_X)))
  {
    return true;
  }
  if (((m1 == ZERO_PAGE_Y) && (m2 == ABSOLUTE_Y)) ||
      ((m1 == ABSOLUTE_Y)  && (m2 == ZERO_PAGE_Y)))
  {
    return true;
  }
  return false;
}
