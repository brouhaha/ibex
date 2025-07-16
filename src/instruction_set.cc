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
  /* ZP_IND       */ 1,
  /* ZP_X_IND     */ 1,
  /* ZP_IND_Y     */ 1,
  /* ABSOLUTE     */ 2,
  /* ABSOLUTE_X   */ 2,
  /* ABSOLUTE_Y   */ 2,
  /* ABSOLUTE_IND */ 2,
  /* ABS_X_IND    */ 2,
  /* RELATIVE     */ 1,
  /* ZP_RELATIVE  */ 2,
  /* RELATIVE_16  */ 2,
  /* ST_VEC_IND_Y */ 1,
};

const magic_enum::containers::array<InstructionSet::Mode, std::string> s_mos_address_mode_prefixes
{
  /* IMPLIED      */ "",
  /* ACCUMULATOR  */ "",
  /* IMMEDIATE    */ "#",
  /* ZERO_PAGE    */ "",
  /* ZERO_PAGE_X  */ "",
  /* ZERO_PAGE_Y  */ "",
  /* ZP_IND       */ "(",
  /* ZP_X_IND     */ "(",
  /* ZP_IND_Y     */ "(",
  /* ABSOLUTE     */ "",
  /* ABSOLUTE_X   */ "",
  /* ABSOLUTE_Y   */ "",
  /* ABSOLUTE_IND */ "(",
  /* ABS_X_IND    */ "(",
  /* RELATIVE     */ "",
  /* ZP_RELATIVE  */ "",
  /* RELATIVE_16  */ "",
  /* ST_VEC_IND_Y */ "(",
};

const magic_enum::containers::array<InstructionSet::Mode, std::string> s_mos_address_mode_suffixes
{
  /* IMPLIED      */ "",
  /* ACCUMULATOR  */ "",
  /* IMMEDIATE    */ "",
  /* ZERO_PAGE    */ "",
  /* ZERO_PAGE_X  */ ",x",
  /* ZERO_PAGE_Y  */ ",y",
  /* ZP_IND       */ ")",
  /* ZP_X_IND     */ ",x)",
  /* ZP_IND_Y     */ "),y",
  /* ABSOLUTE     */ "",
  /* ABSOLUTE_X   */ ",x",
  /* ABSOLUTE_Y   */ ",y",
  /* ABSOLUTE_IND */ ")",
  /* ABS_X_IND    */ ",x)",
  /* RELATIVE     */ "",
  /* ZP_RELATIVE  */ "",
  /* RELATIVE_16  */ "",
  /* ST_VEC_IND_Y */ "),y",
};

const magic_enum::containers::array<InstructionSet::Mode, std::string> s_pal65_address_mode_suffixes
{
  /* IMPLIED      */ "",
  /* ACCUMULATOR  */ "a",
  /* IMMEDIATE    */ "#",
  /* ZERO_PAGE    */ "",
  /* ZERO_PAGE_X  */ "x",
  /* ZERO_PAGE_Y  */ "y",
  /* ZP_IND       */ "@",
  /* ZP_X_IND     */ "x@",
  /* ZP_IND_Y     */ "@y",
  /* ABSOLUTE     */ "",
  /* ABSOLUTE_X   */ "x",
  /* ABSOLUTE_Y   */ "y",
  /* ABSOLUTE_IND */ "@",
  /* ABS_X_IND    */ "x@",
  /* RELATIVE     */ "",
  /* ZP_RELATIVE  */ "",
  /* RELATIVE_16  */ "",
  /* ST_VEC_IND_Y */ "s@y",
};

const std::vector<InstructionSet::Info> s_main_table
{
  { "adc",  BASE,     ADC,  IMMEDIATE,    0x69 },
  { "adc",  BASE,     ADC,  ZERO_PAGE,    0x65 },
  { "adc",  BASE,     ADC,  ZERO_PAGE_X,  0x75 },
  { "adc",  CMOS,     ADC,  ZP_IND,       0x72 },
  { "adc",  BASE,     ADC,  ZP_X_IND,     0x61 },
  { "adc",  BASE,     ADC,  ZP_IND_Y,     0x71 },
  { "adc",  BASE,     ADC,  ABSOLUTE,     0x6d },
  { "adc",  BASE,     ADC,  ABSOLUTE_X,   0x7d },
  { "adc",  BASE,     ADC,  ABSOLUTE_Y,   0x79 },

  { "and",  BASE,     AND,  IMMEDIATE,    0x29 },
  { "and",  BASE,     AND,  ZERO_PAGE,    0x25 },
  { "and",  BASE,     AND,  ZERO_PAGE_X,  0x35 },
  { "and",  CMOS,     AND,  ZP_IND,       0x32 },
  { "and",  BASE,     AND,  ZP_X_IND,     0x21 },
  { "and",  BASE,     AND,  ZP_IND_Y,     0x31 },
  { "and",  BASE,     AND,  ABSOLUTE,     0x2d },
  { "and",  BASE,     AND,  ABSOLUTE_X,   0x3d },
  { "and",  BASE,     AND,  ABSOLUTE_Y,   0x39 },

  { "asl",  BASE,     ASL,  ACCUMULATOR,  0x0a },
  { "asl",  BASE,     ASL,  ZERO_PAGE,    0x06 },
  { "asl",  BASE,     ASL,  ZERO_PAGE_X,  0x16 },
  { "asl",  BASE,     ASL,  ABSOLUTE,     0x0e },
  { "asl",  BASE,     ASL,  ABSOLUTE_X,   0x1e },

  { "asr",  CBM_CMOS, ASR,  ACCUMULATOR,  0x43 },
  { "asr",  CBM_CMOS, ASR,  ZERO_PAGE,    0x44 },
  { "asr",  CBM_CMOS, ASR,  ZERO_PAGE_X,  0x54 },

  { "asw",  CBM_CMOS, ASW,  ABSOLUTE,     0xcb },

  { "aug",  CBM_CMOS, AUG,  IMPLIED,      0x5c }, // 4-byte instruction

  { "bbr0", ROCKWELL, BBR0, ZP_RELATIVE,  0x0f },
  { "bbr1", ROCKWELL, BBR1, ZP_RELATIVE,  0x1f },
  { "bbr2", ROCKWELL, BBR2, ZP_RELATIVE,  0x2f },
  { "bbr3", ROCKWELL, BBR3, ZP_RELATIVE,  0x3f },
  { "bbr4", ROCKWELL, BBR4, ZP_RELATIVE,  0x4f },
  { "bbr5", ROCKWELL, BBR5, ZP_RELATIVE,  0x5f },
  { "bbr6", ROCKWELL, BBR6, ZP_RELATIVE,  0x6f },
  { "bbr7", ROCKWELL, BBR7, ZP_RELATIVE,  0x7f },

  { "bbs0", ROCKWELL, BBS0, ZP_RELATIVE,  0x8f },
  { "bbs1", ROCKWELL, BBS1, ZP_RELATIVE,  0x9f },
  { "bbs2", ROCKWELL, BBS2, ZP_RELATIVE,  0xaf },
  { "bbs3", ROCKWELL, BBS3, ZP_RELATIVE,  0xbf },
  { "bbs4", ROCKWELL, BBS4, ZP_RELATIVE,  0xcf },
  { "bbs5", ROCKWELL, BBS5, ZP_RELATIVE,  0xdf },
  { "bbs6", ROCKWELL, BBS6, ZP_RELATIVE,  0xef },
  { "bbs7", ROCKWELL, BBS7, ZP_RELATIVE,  0xff },

  { "bcc",  BASE,     BCC,  RELATIVE,     0x90 },
  { "bcc",  CBM_CMOS, BCC,  RELATIVE_16,  0x93 },

  { "bcs",  BASE,     BCS,  RELATIVE,     0xb0 },
  { "bcs",  CBM_CMOS, BCS,  RELATIVE_16,  0xb3 },

  { "beq",  BASE,     BEQ,  RELATIVE,     0xf0 },
  { "beq",  CBM_CMOS, BEQ,  RELATIVE_16,  0xf3 },

  { "bit",  CMOS,     BIT,  IMMEDIATE,    0x89 },
  { "bit",  BASE,     BIT,  ZERO_PAGE,    0x24 },
  { "bit",  CMOS,     BIT,  ZERO_PAGE_X,  0x34 },
  { "bit",  BASE,     BIT,  ABSOLUTE,     0x2c },
  { "bit",  CMOS,     BIT,  ABSOLUTE_X,   0x3c },

  { "bmi",  BASE,     BMI,  RELATIVE,     0x30 },
  { "bmi",  CBM_CMOS, BMI,  RELATIVE_16,  0x33 },

  { "bne",  BASE,     BNE,  RELATIVE,     0xd0 },
  { "bne",  CBM_CMOS, BNE,  RELATIVE_16,  0xd3 },

  { "bpl",  BASE,     BPL,  RELATIVE,     0x10 },
  { "bpl",  CBM_CMOS, BPL,  RELATIVE_16,  0x13 },

  { "brk",  BASE,     BRK,  IMPLIED,      0x00 },

  { "bra",  CMOS,     BRA,  RELATIVE,     0x80 }, // Commodore calls this BRU
  { "bra",  CBM_CMOS, BRA,  RELATIVE_16,  0x83 }, // Commodore calls this BRU

  { "bsr",  CBM_CMOS, BSR,  RELATIVE_16,  0x63 },

  { "bvc",  BASE,     BVC,  RELATIVE,     0x50 },
  { "bvc",  CBM_CMOS, BVC,  RELATIVE_16,  0x53 },

  { "bvs",  BASE,     BVS,  RELATIVE,     0x70 },
  { "bvs",  CBM_CMOS, BVS,  RELATIVE_16,  0x73 },

  { "cle",  CBM_CMOS, CLE,  IMPLIED,      0x02 },

  { "clc",  BASE,     CLC,  IMPLIED,      0x18 },

  { "cld",  BASE,     CLD,  IMPLIED,      0xd8 },

  { "cli",  BASE,     CLI,  IMPLIED,      0x58 },

  { "clv",  BASE,     CLV,  IMPLIED,      0xb8 },

  { "cmp",  BASE,     CMP,  IMMEDIATE,    0xc9 },
  { "cmp",  BASE,     CMP,  ZERO_PAGE,    0xc5 },
  { "cmp",  BASE,     CMP,  ZERO_PAGE_X,  0xd5 },
  { "cmp",  CMOS,     CMP,  ZP_IND,       0xd2 },
  { "cmp",  BASE,     CMP,  ZP_X_IND,     0xc1 },
  { "cmp",  BASE,     CMP,  ZP_IND_Y,     0xd1 },
  { "cmp",  BASE,     CMP,  ABSOLUTE,     0xcd },
  { "cmp",  BASE,     CMP,  ABSOLUTE_X,   0xdd },
  { "cmp",  BASE,     CMP,  ABSOLUTE_Y,   0xd9 },

  { "cpx",  BASE,     CPX,  IMMEDIATE,    0xe0 },
  { "cpx",  BASE,     CPX,  ZERO_PAGE,    0xe4 },
  { "cpx",  BASE,     CPX,  ABSOLUTE,     0xec },

  { "cpy",  BASE,     CPY,  IMMEDIATE,    0xc0 },
  { "cpy",  BASE,     CPY,  ZERO_PAGE,    0xc4 },
  { "cpy",  BASE,     CPY,  ABSOLUTE,     0xcc },

  { "cpz",  CBM_CMOS, CPZ,  IMMEDIATE,    0xc2 },
  { "cpz",  CBM_CMOS, CPZ,  ZERO_PAGE,    0xd4 },
  { "cpz",  CBM_CMOS, CPZ,  ABSOLUTE,     0xdc },

  { "dec",  CMOS,     DEC,  ACCUMULATOR,  0x3a },
  { "dec",  BASE,     DEC,  ZERO_PAGE,    0xc6 },
  { "dec",  BASE,     DEC,  ZERO_PAGE_X,  0xd6 },
  { "dec",  BASE,     DEC,  ABSOLUTE,     0xce },
  { "dec",  BASE,     DEC,  ABSOLUTE_X,   0xde },

  { "dew",  CBM_CMOS, DEW,  ZERO_PAGE,    0xc3 },

  { "dex",  BASE,     DEX,  IMPLIED,      0xca },

  { "dey",  BASE,     DEY,  IMPLIED,      0x88 },

  { "dez",  CBM_CMOS, DEZ,  IMPLIED,      0x3b },

  { "eor",  BASE,     EOR,  IMMEDIATE,    0x49 },
  { "eor",  BASE,     EOR,  ZERO_PAGE,    0x45 },
  { "eor",  BASE,     EOR,  ZERO_PAGE_X,  0x55 },
  { "eor",  CMOS,     EOR,  ZP_IND,       0x52 },
  { "eor",  BASE,     EOR,  ZP_X_IND,     0x41 },
  { "eor",  BASE,     EOR,  ZP_IND_Y,     0x51 },
  { "eor",  BASE,     EOR,  ABSOLUTE,     0x4d },
  { "eor",  BASE,     EOR,  ABSOLUTE_X,   0x5d },
  { "eor",  BASE,     EOR,  ABSOLUTE_Y,   0x59 },

  { "inc",  CMOS,     INC,  ACCUMULATOR,  0x1a },
  { "inc",  BASE,     INC,  ZERO_PAGE,    0xe6 },
  { "inc",  BASE,     INC,  ZERO_PAGE_X,  0xf6 },
  { "inc",  BASE,     INC,  ABSOLUTE,     0xee },
  { "inc",  BASE,     INC,  ABSOLUTE_X,   0xfe },

  { "inw",  CBM_CMOS, INW,  ZERO_PAGE,    0xe3 },

  { "inx",  BASE,     INX,  IMPLIED,      0xe8 },

  { "iny",  BASE,     INY,  IMPLIED,      0xc8 },

  { "inz",  CBM_CMOS, INZ,  IMPLIED,      0x1b },

  { "jmp",  BASE,     JMP,  ABSOLUTE,     0x4c },
  { "jmp",  BASE,     JMP,  ABSOLUTE_IND, 0x6c },
  { "jmp",  CMOS,     JMP,  ABS_X_IND,    0x7c },

  { "jsr",  BASE,     JSR,  ABSOLUTE,     0x20 },
  { "jsr",  CBM_CMOS, JSR,  ABSOLUTE_IND, 0x22 },
  { "jsr",  CBM_CMOS, JSR,  ABS_X_IND,    0x23 },

  { "lda",  BASE,     LDA,  IMMEDIATE,    0xa9 },
  { "lda",  BASE,     LDA,  ZERO_PAGE,    0xa5 },
  { "lda",  BASE,     LDA,  ZERO_PAGE_X,  0xb5 },
  { "lda",  CMOS,     LDA,  ZP_IND,       0xb2 },
  { "lda",  BASE,     LDA,  ZP_X_IND,     0xa1 },
  { "lda",  BASE,     LDA,  ZP_IND_Y,     0xb1 },
  { "lda",  BASE,     LDA,  ABSOLUTE,     0xad },
  { "lda",  BASE,     LDA,  ABSOLUTE_X,   0xbd },
  { "lda",  BASE,     LDA,  ABSOLUTE_Y,   0xb9 },
  { "lda",  CBM_CMOS, LDA,  ST_VEC_IND_Y, 0xe2 },

  { "ldx",  BASE,     LDX,  IMMEDIATE,    0xa2 },
  { "ldx",  BASE,     LDX,  ZERO_PAGE,    0xa6 },
  { "ldx",  BASE,     LDX,  ZERO_PAGE_Y,  0xb6 },
  { "ldx",  BASE,     LDX,  ABSOLUTE,     0xae },
  { "ldx",  BASE,     LDX,  ABSOLUTE_Y,   0xbe },

  { "ldy",  BASE,     LDY,  IMMEDIATE,    0xa0 },
  { "ldy",  BASE,     LDY,  ZERO_PAGE,    0xa4 },
  { "ldy",  BASE,     LDY,  ZERO_PAGE_X,  0xb4 },
  { "ldy",  BASE,     LDY,  ABSOLUTE,     0xac },
  { "ldy",  BASE,     LDY,  ABSOLUTE_X,   0xbc },

  { "ldz",  CBM_CMOS, LDZ,  IMMEDIATE,    0xa3 },
  { "ldz",  CBM_CMOS, LDZ,  ABSOLUTE,     0xab },
  { "ldz",  CBM_CMOS, LDZ,  ABSOLUTE_X,   0xbb },

  { "lsr",  BASE,     LSR,  ACCUMULATOR,  0x4a },
  { "lsr",  BASE,     LSR,  ZERO_PAGE,    0x46 },
  { "lsr",  BASE,     LSR,  ZERO_PAGE_X,  0x56 },
  { "lsr",  BASE,     LSR,  ABSOLUTE,     0x4e },
  { "lsr",  BASE,     LSR,  ABSOLUTE_X,   0x5e },

  { "neg",  CBM_CMOS, NEG,  ACCUMULATOR,  0x42 },

  { "nop",  BASE,     NOP,  IMPLIED,      0xea },

  { "ora",  BASE,     ORA,  IMMEDIATE,    0x09 },
  { "ora",  BASE,     ORA,  ZERO_PAGE,    0x05 },
  { "ora",  BASE,     ORA,  ZERO_PAGE_X,  0x15 },
  { "ora",  CMOS,     ORA,  ZP_IND,       0x12 },
  { "ora",  BASE,     ORA,  ZP_X_IND,     0x01 },
  { "ora",  BASE,     ORA,  ZP_IND_Y,     0x11 },
  { "ora",  BASE,     ORA,  ABSOLUTE,     0x0d },
  { "ora",  BASE,     ORA,  ABSOLUTE_X,   0x1d },
  { "ora",  BASE,     ORA,  ABSOLUTE_Y,   0x19 },

  { "pha",  BASE,     PHA,  IMPLIED,      0x48 },

  { "php",  BASE,     PHP,  IMPLIED,      0x08 },

  { "phw",  CBM_CMOS, PHW,  IMMEDIATE,    0xf4 },
  { "phw",  CBM_CMOS, PHW,  ABSOLUTE,     0xfc },

  { "phx",  CMOS,     PHX,  IMPLIED,      0xda },

  { "phy",  CMOS,     PHY,  IMPLIED,      0x5a },

  { "phz",  CBM_CMOS, PHZ,  IMPLIED,      0xdb },

  { "pla",  BASE,     PLA,  IMPLIED,      0x68 },

  { "plp",  BASE,     PLP,  IMPLIED,      0x28 },

  { "plx",  CMOS,     PLX,  IMPLIED,      0xfa },

  { "ply",  CMOS,     PLY,  IMPLIED,      0x7a },

  { "plz",  CBM_CMOS, PLZ,  IMPLIED,      0xfb },

  { "rmb0", ROCKWELL, RMB0, ZERO_PAGE,    0x07 },
  { "rmb1", ROCKWELL, RMB1, ZERO_PAGE,    0x17 },
  { "rmb2", ROCKWELL, RMB2, ZERO_PAGE,    0x27 },
  { "rmb3", ROCKWELL, RMB3, ZERO_PAGE,    0x37 },
  { "rmb4", ROCKWELL, RMB4, ZERO_PAGE,    0x47 },
  { "rmb5", ROCKWELL, RMB5, ZERO_PAGE,    0x57 },
  { "rmb6", ROCKWELL, RMB6, ZERO_PAGE,    0x67 },
  { "rmb7", ROCKWELL, RMB7, ZERO_PAGE,    0x77 },

  { "rol",  BASE,     ROL,  ACCUMULATOR,  0x2a },
  { "rol",  BASE,     ROL,  ZERO_PAGE,    0x26 },
  { "rol",  BASE,     ROL,  ZERO_PAGE_X,  0x36 },
  { "rol",  BASE,     ROL,  ABSOLUTE,     0x2e },
  { "rol",  BASE,     ROL,  ABSOLUTE_X,   0x3e },

  // very early 6502 didn't have ROR
  { "ror",  BASE,     ROR,  ACCUMULATOR,  0x6a },
  { "ror",  BASE,     ROR,  ZERO_PAGE,    0x66 },
  { "ror",  BASE,     ROR,  ZERO_PAGE_X,  0x76 },
  { "ror",  BASE,     ROR,  ABSOLUTE,     0x6e },
  { "ror",  BASE,     ROR,  ABSOLUTE_X,   0x7e },

  { "row",  CBM_CMOS, ROW,  ABSOLUTE,     0xeb },

  { "rti",  BASE,     RTI,  IMPLIED,      0x40 },

  { "rtn",  CBM_CMOS, RTN,  IMMEDIATE,    0x62 },

  { "rts",  BASE,     RTS,  IMPLIED,      0x60 },

  { "sbc",  BASE,     SBC,  IMMEDIATE,    0xe9 },
  { "sbc",  BASE,     SBC,  ZERO_PAGE,    0xe5 },
  { "sbc",  BASE,     SBC,  ZERO_PAGE_X,  0xf5 },
  { "sbc",  CMOS,     SBC,  ZP_IND,       0xf2 },
  { "sbc",  BASE,     SBC,  ZP_X_IND,     0xe1 },
  { "sbc",  BASE,     SBC,  ZP_IND_Y,     0xf1 },
  { "sbc",  BASE,     SBC,  ABSOLUTE,     0xed },
  { "sbc",  BASE,     SBC,  ABSOLUTE_X,   0xfd },
  { "sbc",  BASE,     SBC,  ABSOLUTE_Y,   0xf9 },

  { "sec",  BASE,     SEC,  IMPLIED,      0x38 },

  { "sed",  BASE,     SED,  IMPLIED,      0xf8 },

  { "see",  CBM_CMOS, SEE,  IMPLIED,      0x03 },

  { "sei",  BASE,     SEI,  IMPLIED,      0x78 },

  { "smb0", ROCKWELL, SMB0, ZERO_PAGE,    0x87 },
  { "smb1", ROCKWELL, SMB1, ZERO_PAGE,    0x97 },
  { "smb2", ROCKWELL, SMB2, ZERO_PAGE,    0xa7 },
  { "smb3", ROCKWELL, SMB3, ZERO_PAGE,    0xb7 },
  { "smb4", ROCKWELL, SMB4, ZERO_PAGE,    0xc7 },
  { "smb5", ROCKWELL, SMB5, ZERO_PAGE,    0xd7 },
  { "smb6", ROCKWELL, SMB6, ZERO_PAGE,    0xe7 },
  { "smb7", ROCKWELL, SMB7, ZERO_PAGE,    0xf7 },

  { "sta",  BASE,     STA,  ZERO_PAGE,    0x85 },
  { "sta",  BASE,     STA,  ZERO_PAGE_X,  0x95 },
  { "sta",  CMOS,     STA,  ZP_IND,       0x92 },
  { "sta",  BASE,     STA,  ZP_X_IND,     0x81 },
  { "sta",  BASE,     STA,  ZP_IND_Y,     0x91 },
  { "sta",  BASE,     STA,  ABSOLUTE,     0x8d },
  { "sta",  BASE,     STA,  ABSOLUTE_X,   0x9d },
  { "sta",  BASE,     STA,  ABSOLUTE_Y,   0x99 },
  { "sta",  CBM_CMOS, STA,  ST_VEC_IND_Y, 0x82 },

  { "stp",  WDC_CMOS, STP,  IMPLIED,      0xdb },

  { "stx",  BASE,     STX,  ZERO_PAGE,    0x86 },
  { "stx",  BASE,     STX,  ZERO_PAGE_Y,  0x96 },
  { "stx",  BASE,     STX,  ABSOLUTE,     0x8e },
  { "stx",  CBM_CMOS, STX,  ABSOLUTE_Y,   0x9b },

  { "sty",  BASE,     STY,  ZERO_PAGE,    0x84 },
  { "sty",  BASE,     STY,  ZERO_PAGE_X,  0x94 },
  { "sty",  BASE,     STY,  ABSOLUTE,     0x8c },
  { "sty",  CBM_CMOS, STY,  ABSOLUTE_X,   0x8b },

  { "stz",  CMOS,     STZ,  ZERO_PAGE,    0x64 },
  { "stz",  CMOS,     STZ,  ZERO_PAGE_X,  0x74 },
  { "stz",  CMOS,     STZ,  ABSOLUTE,     0x9c },
  { "stz",  CMOS,     STZ,  ABSOLUTE_X,   0x9e },

  { "tab",  CBM_CMOS, TAB,  IMPLIED,      0x5b },

  { "tax",  BASE,     TAX,  IMPLIED,      0xaa },

  { "tay",  BASE,     TAY,  IMPLIED,      0xa8 },

  { "taz",  CBM_CMOS, TAZ,  IMPLIED,      0x4b },

  { "tba",  CBM_CMOS, TBA,  IMPLIED,      0x7b },

  { "trb",  CMOS,     TRB,  ZERO_PAGE,    0x14 },
  { "trb",  CMOS,     TRB,  ABSOLUTE,     0x1c },

  { "tsb",  CMOS,     TSB,  ZERO_PAGE,    0x04 },
  { "tsb",  CMOS,     TSB,  ABSOLUTE,     0x0c },

  { "tsx",  BASE,     TSX,  IMPLIED,      0xba },

  { "tsy",  CBM_CMOS, TSY,  IMPLIED,      0x0b },

  { "txa",  BASE,     TXA,  IMPLIED,      0x8a },

  { "txs",  BASE,     TXS,  IMPLIED,      0x9a },

  { "tya",  BASE,     TYA,  IMPLIED,      0x98 },

  { "tys",  CBM_CMOS, TYS,  IMPLIED,      0x2b },

  { "wai",  WDC_CMOS, WAI,  IMPLIED,      0xcb },

  { "tza",  CBM_CMOS, TZA,  IMPLIED,      0x6b },
};

InstructionSet::UnrecognizedMnemonic::UnrecognizedMnemonic(const std::string& mnemonic):
  std::runtime_error(std::format("unrecognized mnemonic {}", mnemonic))
{
}

std::shared_ptr<InstructionSet> InstructionSet::create(const Sets& sets)
{
  auto p = new InstructionSet(sets);
  return std::shared_ptr<InstructionSet>(p);
}

InstructionSet::InstructionSet(const Sets& sets)
{
  for (const auto& info: s_main_table)
  {
    if (! sets.test(info.set))
    {
      continue;
    }
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

std::ostream& InstructionSet::dump_opcodes(std::ostream& os)
{
  unsigned count = 0;
  os << "    ";
  for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
  {
    os << std::format("  {:02x} ", lsd);
  }
  os << "\n";
  os << "    ";
  for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
  {
    os << std::format(" ----");
  }
  os << "\n";
  for (std::uint8_t msd = 0; msd < 0x10; msd++)
  {
    os << std::format("{:02x} |", msd << 4);
    for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
    {
      std::uint8_t opcode = (msd << 4) | lsd;
      const Info* info = m_by_opcode[opcode];
      if (info)
      {
	os << std::format(" {:4}", info->mnemonic);
	++count;
      }
      else
      {
	os << "     ";
      }
    }
    os << "\n";
  }
  os << "\n";
  os << std::format("InstructionSet:: {} opcodes\n", count);
  return os;
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
