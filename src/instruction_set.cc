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

const magic_enum::containers::array<InstructionSet::Mode, std::uint8_t> s_address_mode_added_cycles
{
  /* IMPLIED      */ 0,
  /* ACCUMULATOR  */ 0,
  /* IMMEDIATE    */ 1,
  /* ZERO_PAGE    */ 2,
  /* ZERO_PAGE_X  */ 3,
  /* ZERO_PAGE_Y  */ 3,
  /* ZP_IND       */ 4,
  /* ZP_X_IND     */ 5,
  /* ZP_IND_Y     */ 4,
  /* ABSOLUTE     */ 3,
  /* ABSOLUTE_X   */ 3,
  /* ABSOLUTE_Y   */ 3,
  /* ABSOLUTE_IND */ 5,
  /* ABS_X_IND    */ 5,
  /* RELATIVE     */ 0,
  /* ZP_RELATIVE  */ 2,
  /* RELATIVE_16  */ 2, // ?
  /* ST_VEC_IND_Y */ 1, // ?
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

static constexpr std::uint8_t UNKNOWN_CYCLE_COUNT = 0xff;

const std::vector<InstructionSet::Info> s_main_table
{
  //                                               page   NMOS
  //                                               cross  RMW    65C02
  //                                               extra  extra  extra
  //                                               cycle  cycle  cycle
  //                                               -----  -----  -----
  { "adc",  BASE,     ADC,  IMMEDIATE,    0x69, 1, false, false, false, },
  { "adc",  BASE,     ADC,  ZERO_PAGE,    0x65, 1, false, false, false, },
  { "adc",  BASE,     ADC,  ZERO_PAGE_X,  0x75, 1, false, false, false, },
  { "adc",  CMOS,     ADC,  ZP_IND,       0x72, 1, false, false, false, },
  { "adc",  BASE,     ADC,  ZP_X_IND,     0x61, 1, false, false, false, },
  { "adc",  BASE,     ADC,  ZP_IND_Y,     0x71, 1, true,  false, false, },
  { "adc",  BASE,     ADC,  ABSOLUTE,     0x6d, 1, false, false, false, },
  { "adc",  BASE,     ADC,  ABSOLUTE_X,   0x7d, 1, true,  false, false, },
  { "adc",  BASE,     ADC,  ABSOLUTE_Y,   0x79, 1, true,  false, false, },

  { "and",  BASE,     AND,  IMMEDIATE,    0x29, 1, false, false, false, },
  { "and",  BASE,     AND,  ZERO_PAGE,    0x25, 1, false, false, false, },
  { "and",  BASE,     AND,  ZERO_PAGE_X,  0x35, 1, false, false, false, },
  { "and",  CMOS,     AND,  ZP_IND,       0x32, 1, false, false, false, },
  { "and",  BASE,     AND,  ZP_X_IND,     0x21, 1, false, false, false, },
  { "and",  BASE,     AND,  ZP_IND_Y,     0x31, 1, true,  false, false, },
  { "and",  BASE,     AND,  ABSOLUTE,     0x2d, 1, false, false, false, },
  { "and",  BASE,     AND,  ABSOLUTE_X,   0x3d, 1, true,  false, false, },
  { "and",  BASE,     AND,  ABSOLUTE_Y,   0x39, 1, true,  false, false, },

  { "asl",  BASE,     ASL,  ACCUMULATOR,  0x0a, 2, false, false, false, },
  { "asl",  BASE,     ASL,  ZERO_PAGE,    0x06, 3, false, false, false, },
  { "asl",  BASE,     ASL,  ZERO_PAGE_X,  0x16, 3, false, false, false, },
  { "asl",  BASE,     ASL,  ABSOLUTE,     0x0e, 3, false, false, false, },
  { "asl",  BASE,     ASL,  ABSOLUTE_X,   0x1e, 3, true,  true,  false  },

  { "asr",  CBM_CMOS, ASR,  ACCUMULATOR,  0x43, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "asr",  CBM_CMOS, ASR,  ZERO_PAGE,    0x44, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "asr",  CBM_CMOS, ASR,  ZERO_PAGE_X,  0x54, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "asw",  CBM_CMOS, ASW,  ABSOLUTE,     0xcb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "aug",  CBM_CMOS, AUG,  IMPLIED,      0x5c, UNKNOWN_CYCLE_COUNT, false, false, false,}, // 4-byte instruction

  { "bbr0", ROCKWELL, BBR0, ZP_RELATIVE,  0x0f, 3, false, false, false, },
  { "bbr1", ROCKWELL, BBR1, ZP_RELATIVE,  0x1f, 3, false, false, false, },
  { "bbr2", ROCKWELL, BBR2, ZP_RELATIVE,  0x2f, 3, false, false, false, },
  { "bbr3", ROCKWELL, BBR3, ZP_RELATIVE,  0x3f, 3, false, false, false, },
  { "bbr4", ROCKWELL, BBR4, ZP_RELATIVE,  0x4f, 3, false, false, false, },
  { "bbr5", ROCKWELL, BBR5, ZP_RELATIVE,  0x5f, 3, false, false, false, },
  { "bbr6", ROCKWELL, BBR6, ZP_RELATIVE,  0x6f, 3, false, false, false, },
  { "bbr7", ROCKWELL, BBR7, ZP_RELATIVE,  0x7f, 3, false, false, false, },

  { "bbs0", ROCKWELL, BBS0, ZP_RELATIVE,  0x8f, 3, false, false, false, },
  { "bbs1", ROCKWELL, BBS1, ZP_RELATIVE,  0x9f, 3, false, false, false, },
  { "bbs2", ROCKWELL, BBS2, ZP_RELATIVE,  0xaf, 3, false, false, false, },
  { "bbs3", ROCKWELL, BBS3, ZP_RELATIVE,  0xbf, 3, false, false, false, },
  { "bbs4", ROCKWELL, BBS4, ZP_RELATIVE,  0xcf, 3, false, false, false, },
  { "bbs5", ROCKWELL, BBS5, ZP_RELATIVE,  0xdf, 3, false, false, false, },
  { "bbs6", ROCKWELL, BBS6, ZP_RELATIVE,  0xef, 3, false, false, false, },
  { "bbs7", ROCKWELL, BBS7, ZP_RELATIVE,  0xff, 3, false, false, false, },

  { "bcc",  BASE,     BCC,  RELATIVE,     0x90, 2, false, false, false, },
  { "bcc",  CBM_CMOS, BCC,  RELATIVE_16,  0x93, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "bcs",  BASE,     BCS,  RELATIVE,     0xb0, 2, false, false, false, },
  { "bcs",  CBM_CMOS, BCS,  RELATIVE_16,  0xb3, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "beq",  BASE,     BEQ,  RELATIVE,     0xf0, 2, false, false, false, },
  { "beq",  CBM_CMOS, BEQ,  RELATIVE_16,  0xf3, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "bit",  CMOS,     BIT,  IMMEDIATE,    0x89, 1, false, false, false, },
  { "bit",  BASE,     BIT,  ZERO_PAGE,    0x24, 1, false, false, false, },
  { "bit",  CMOS,     BIT,  ZERO_PAGE_X,  0x34, 1, false, false, false, },
  { "bit",  BASE,     BIT,  ABSOLUTE,     0x2c, 1, false, false, false, },
  { "bit",  CMOS,     BIT,  ABSOLUTE_X,   0x3c, 1, true,  false, false, },

  { "bmi",  BASE,     BMI,  RELATIVE,     0x30, 2, false, false, false, },
  { "bmi",  CBM_CMOS, BMI,  RELATIVE_16,  0x33, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "bne",  BASE,     BNE,  RELATIVE,     0xd0, 2, false, false, false, },
  { "bne",  CBM_CMOS, BNE,  RELATIVE_16,  0xd3, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "bpl",  BASE,     BPL,  RELATIVE,     0x10, 2, false, false, false, },
  { "bpl",  CBM_CMOS, BPL,  RELATIVE_16,  0x13, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "brk",  BASE,     BRK,  IMPLIED,      0x00, 7, false, false, false, },

  { "bra",  CMOS,     BRA,  RELATIVE,     0x80, 2, false, false, false, }, // Commodore calls this BRU
  { "bra",  CBM_CMOS, BRA,  RELATIVE_16,  0x83, UNKNOWN_CYCLE_COUNT, false, false, false, }, // Commodore calls this BRU

  { "bsr",  CBM_CMOS, BSR,  RELATIVE_16,  0x63, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "bvc",  BASE,     BVC,  RELATIVE,     0x50, 2, false, false, false, },
  { "bvc",  CBM_CMOS, BVC,  RELATIVE_16,  0x53, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "bvs",  BASE,     BVS,  RELATIVE,     0x70, 2, false, false, false, },
  { "bvs",  CBM_CMOS, BVS,  RELATIVE_16,  0x73, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "cle",  CBM_CMOS, CLE,  IMPLIED,      0x02, 2, false, false, false, },

  { "clc",  BASE,     CLC,  IMPLIED,      0x18, 2, false, false, false, },

  { "cld",  BASE,     CLD,  IMPLIED,      0xd8, 2, false, false, false, },

  { "cli",  BASE,     CLI,  IMPLIED,      0x58, 2, false, false, false, },

  { "clv",  BASE,     CLV,  IMPLIED,      0xb8, 2, false, false, false, },

  { "cmp",  BASE,     CMP,  IMMEDIATE,    0xc9, 1, false, false, false, },
  { "cmp",  BASE,     CMP,  ZERO_PAGE,    0xc5, 1, false, false, false, },
  { "cmp",  BASE,     CMP,  ZERO_PAGE_X,  0xd5, 1, false, false, false, },
  { "cmp",  CMOS,     CMP,  ZP_IND,       0xd2, 1, false, false, false, },
  { "cmp",  BASE,     CMP,  ZP_X_IND,     0xc1, 1, false, false, false, },
  { "cmp",  BASE,     CMP,  ZP_IND_Y,     0xd1, 1, true,  false, false, },
  { "cmp",  BASE,     CMP,  ABSOLUTE,     0xcd, 1, false, false, false, },
  { "cmp",  BASE,     CMP,  ABSOLUTE_X,   0xdd, 1, true,  false, false, },
  { "cmp",  BASE,     CMP,  ABSOLUTE_Y,   0xd9, 1, true,  false, false, },

  { "cpx",  BASE,     CPX,  IMMEDIATE,    0xe0, 1, false, false, false, },
  { "cpx",  BASE,     CPX,  ZERO_PAGE,    0xe4, 1, false, false, false, },
  { "cpx",  BASE,     CPX,  ABSOLUTE,     0xec, 1, false, false, false, },

  { "cpy",  BASE,     CPY,  IMMEDIATE,    0xc0, 1, false, false, false, },
  { "cpy",  BASE,     CPY,  ZERO_PAGE,    0xc4, 1, false, false, false, },
  { "cpy",  BASE,     CPY,  ABSOLUTE,     0xcc, 1, false, false, false, },

  { "cpz",  CBM_CMOS, CPZ,  IMMEDIATE,    0xc2, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "cpz",  CBM_CMOS, CPZ,  ZERO_PAGE,    0xd4, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "cpz",  CBM_CMOS, CPZ,  ABSOLUTE,     0xdc, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "dec",  CMOS,     DEC,  ACCUMULATOR,  0x3a, 2, false, false, false, },
  { "dec",  BASE,     DEC,  ZERO_PAGE,    0xc6, 3, false, false, false, },
  { "dec",  BASE,     DEC,  ZERO_PAGE_X,  0xd6, 3, false, false, false, },
  { "dec",  BASE,     DEC,  ABSOLUTE,     0xce, 3, false, false, false, },
  { "dec",  BASE,     DEC,  ABSOLUTE_X,   0xde, 3, true,  true,  false, },

  { "dew",  CBM_CMOS, DEW,  ZERO_PAGE,    0xc3, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "dex",  BASE,     DEX,  IMPLIED,      0xca, 2, false, false, false, },

  { "dey",  BASE,     DEY,  IMPLIED,      0x88, 2, false, false, false, },

  { "dez",  CBM_CMOS, DEZ,  IMPLIED,      0x3b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "eor",  BASE,     EOR,  IMMEDIATE,    0x49, 1, false, false, false, },
  { "eor",  BASE,     EOR,  ZERO_PAGE,    0x45, 1, false, false, false, },
  { "eor",  BASE,     EOR,  ZERO_PAGE_X,  0x55, 1, false, false, false, },
  { "eor",  CMOS,     EOR,  ZP_IND,       0x52, 1, false, false, false, },
  { "eor",  BASE,     EOR,  ZP_X_IND,     0x41, 1, false, false, false, },
  { "eor",  BASE,     EOR,  ZP_IND_Y,     0x51, 1, true,  false, false, },
  { "eor",  BASE,     EOR,  ABSOLUTE,     0x4d, 1, false, false, false, },
  { "eor",  BASE,     EOR,  ABSOLUTE_X,   0x5d, 1, true,  false, false, },
  { "eor",  BASE,     EOR,  ABSOLUTE_Y,   0x59, 1, true,  false, false, },

  { "inc",  CMOS,     INC,  ACCUMULATOR,  0x1a, 2, false, false, false, },
  { "inc",  BASE,     INC,  ZERO_PAGE,    0xe6, 3, false, false, false, },
  { "inc",  BASE,     INC,  ZERO_PAGE_X,  0xf6, 3, false, false, false, },
  { "inc",  BASE,     INC,  ABSOLUTE,     0xee, 3, false, false, false, },
  { "inc",  BASE,     INC,  ABSOLUTE_X,   0xfe, 3, true,  true,  false, },

  { "inw",  CBM_CMOS, INW,  ZERO_PAGE,    0xe3, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "inx",  BASE,     INX,  IMPLIED,      0xe8, 2, false, false, false, },

  { "iny",  BASE,     INY,  IMPLIED,      0xc8, 2, false, false, false, },

  { "inz",  CBM_CMOS, INZ,  IMPLIED,      0x1b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "jmp",  BASE,     JMP,  ABSOLUTE,     0x4c, 0, false, false, false, },
  { "jmp",  BASE,     JMP,  ABSOLUTE_IND, 0x6c, 0, false, false, true,  },
  { "jmp",  CMOS,     JMP,  ABS_X_IND,    0x7c, 0, false, false, true,  },

  { "jsr",  BASE,     JSR,  ABSOLUTE,     0x20, 3, false, false, false, },
  { "jsr",  CBM_CMOS, JSR,  ABSOLUTE_IND, 0x22, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "jsr",  CBM_CMOS, JSR,  ABS_X_IND,    0x23, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "lda",  BASE,     LDA,  IMMEDIATE,    0xa9, 1, false, false, false, },
  { "lda",  BASE,     LDA,  ZERO_PAGE,    0xa5, 1, false, false, false, },
  { "lda",  BASE,     LDA,  ZERO_PAGE_X,  0xb5, 1, false, false, false, },
  { "lda",  CMOS,     LDA,  ZP_IND,       0xb2, 1, false, false, false, },
  { "lda",  BASE,     LDA,  ZP_X_IND,     0xa1, 1, false, false, false, },
  { "lda",  BASE,     LDA,  ZP_IND_Y,     0xb1, 1, true,  false, false, },
  { "lda",  BASE,     LDA,  ABSOLUTE,     0xad, 1, false, false, false, },
  { "lda",  BASE,     LDA,  ABSOLUTE_X,   0xbd, 1, true,  false, false, },
  { "lda",  BASE,     LDA,  ABSOLUTE_Y,   0xb9, 1, true,  false, false, },
  { "lda",  CBM_CMOS, LDA,  ST_VEC_IND_Y, 0xe2, 1, false, false, false, },

  { "ldx",  BASE,     LDX,  IMMEDIATE,    0xa2, 1, false, false, false, },
  { "ldx",  BASE,     LDX,  ZERO_PAGE,    0xa6, 1, false, false, false, },
  { "ldx",  BASE,     LDX,  ZERO_PAGE_Y,  0xb6, 1, false, false, false, },
  { "ldx",  BASE,     LDX,  ABSOLUTE,     0xae, 1, false, false, false, },
  { "ldx",  BASE,     LDX,  ABSOLUTE_Y,   0xbe, 1, true,  false, false, },

  { "ldy",  BASE,     LDY,  IMMEDIATE,    0xa0, 1, false, false, false, },
  { "ldy",  BASE,     LDY,  ZERO_PAGE,    0xa4, 1, false, false, false, },
  { "ldy",  BASE,     LDY,  ZERO_PAGE_X,  0xb4, 1, false, false, false, },
  { "ldy",  BASE,     LDY,  ABSOLUTE,     0xac, 1, false, false, false, },
  { "ldy",  BASE,     LDY,  ABSOLUTE_X,   0xbc, 1, true,  false, false, },

  { "ldz",  CBM_CMOS, LDZ,  IMMEDIATE,    0xa3, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "ldz",  CBM_CMOS, LDZ,  ABSOLUTE,     0xab, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "ldz",  CBM_CMOS, LDZ,  ABSOLUTE_X,   0xbb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "lsr",  BASE,     LSR,  ACCUMULATOR,  0x4a, 2, false, false, false, },
  { "lsr",  BASE,     LSR,  ZERO_PAGE,    0x46, 3, false, false, false, },
  { "lsr",  BASE,     LSR,  ZERO_PAGE_X,  0x56, 3, false, false, false, },
  { "lsr",  BASE,     LSR,  ABSOLUTE,     0x4e, 3, false, false, false, },
  { "lsr",  BASE,     LSR,  ABSOLUTE_X,   0x5e, 3, true,  true,  false, },

  { "neg",  CBM_CMOS, NEG,  ACCUMULATOR,  0x42, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "nop",  BASE,     NOP,  IMPLIED,      0xea, 2, false, false, false, },

  { "ora",  BASE,     ORA,  IMMEDIATE,    0x09, 1, false, false, false, },
  { "ora",  BASE,     ORA,  ZERO_PAGE,    0x05, 1, false, false, false, },
  { "ora",  BASE,     ORA,  ZERO_PAGE_X,  0x15, 1, false, false, false, },
  { "ora",  CMOS,     ORA,  ZP_IND,       0x12, 1, false, false, false, },
  { "ora",  BASE,     ORA,  ZP_X_IND,     0x01, 1, false, false, false, },
  { "ora",  BASE,     ORA,  ZP_IND_Y,     0x11, 1, true,  false, false, },
  { "ora",  BASE,     ORA,  ABSOLUTE,     0x0d, 1, false, false, false, },
  { "ora",  BASE,     ORA,  ABSOLUTE_X,   0x1d, 1, true,  false, false, },
  { "ora",  BASE,     ORA,  ABSOLUTE_Y,   0x19, 1, true,  false, false, },

  { "pha",  BASE,     PHA,  IMPLIED,      0x48, 3, false, false, false, },

  { "php",  BASE,     PHP,  IMPLIED,      0x08, 3, false, false, false, },

  { "phw",  CBM_CMOS, PHW,  IMMEDIATE,    0xf4, UNKNOWN_CYCLE_COUNT, false, false, false, },
  { "phw",  CBM_CMOS, PHW,  ABSOLUTE,     0xfc, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "phx",  CMOS,     PHX,  IMPLIED,      0xda, 3, false, false, false, },

  { "phy",  CMOS,     PHY,  IMPLIED,      0x5a, 3, false, false, false, },

  { "phz",  CBM_CMOS, PHZ,  IMPLIED,      0xdb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "pla",  BASE,     PLA,  IMPLIED,      0x68, 4, false, false, false, },

  { "plp",  BASE,     PLP,  IMPLIED,      0x28, 4, false, false, false, },

  { "plx",  CMOS,     PLX,  IMPLIED,      0xfa, 4, false, false, false, },

  { "ply",  CMOS,     PLY,  IMPLIED,      0x7a, 4, false, false, false, },

  { "plz",  CBM_CMOS, PLZ,  IMPLIED,      0xfb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "rmb0", ROCKWELL, RMB0, ZERO_PAGE,    0x07, 3, false, false, false, },
  { "rmb1", ROCKWELL, RMB1, ZERO_PAGE,    0x17, 3, false, false, false, },
  { "rmb2", ROCKWELL, RMB2, ZERO_PAGE,    0x27, 3, false, false, false, },
  { "rmb3", ROCKWELL, RMB3, ZERO_PAGE,    0x37, 3, false, false, false, },
  { "rmb4", ROCKWELL, RMB4, ZERO_PAGE,    0x47, 3, false, false, false, },
  { "rmb5", ROCKWELL, RMB5, ZERO_PAGE,    0x57, 3, false, false, false, },
  { "rmb6", ROCKWELL, RMB6, ZERO_PAGE,    0x67, 3, false, false, false, },
  { "rmb7", ROCKWELL, RMB7, ZERO_PAGE,    0x77, 3, false, false, false, },

  { "rol",  BASE,     ROL,  ACCUMULATOR,  0x2a, 2, false, false, false, },
  { "rol",  BASE,     ROL,  ZERO_PAGE,    0x26, 3, false, false, false, },
  { "rol",  BASE,     ROL,  ZERO_PAGE_X,  0x36, 3, false, false, false, },
  { "rol",  BASE,     ROL,  ABSOLUTE,     0x2e, 3, false, false, false, },
  { "rol",  BASE,     ROL,  ABSOLUTE_X,   0x3e, 3, true,  true,  false, },

  // very early 6502 didn't have ROR
  { "ror",  BASE,     ROR,  ACCUMULATOR,  0x6a, 2, false, false, false, },
  { "ror",  BASE,     ROR,  ZERO_PAGE,    0x66, 3, false, false, false, },
  { "ror",  BASE,     ROR,  ZERO_PAGE_X,  0x76, 3, false, false, false, },
  { "ror",  BASE,     ROR,  ABSOLUTE,     0x6e, 3, false, false, false, },
  { "ror",  BASE,     ROR,  ABSOLUTE_X,   0x7e, 3, true,  true,  false, },

  { "row",  CBM_CMOS, ROW,  ABSOLUTE,     0xeb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "rti",  BASE,     RTI,  IMPLIED,      0x40, 6, false, false, false, },

  { "rtn",  CBM_CMOS, RTN,  IMMEDIATE,    0x62, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "rts",  BASE,     RTS,  IMPLIED,      0x60, 6, false, false, false, },

  { "sbc",  BASE,     SBC,  IMMEDIATE,    0xe9, 1, false, false, false, },
  { "sbc",  BASE,     SBC,  ZERO_PAGE,    0xe5, 1, false, false, false, },
  { "sbc",  BASE,     SBC,  ZERO_PAGE_X,  0xf5, 1, false, false, false, },
  { "sbc",  CMOS,     SBC,  ZP_IND,       0xf2, 1, false, false, false, },
  { "sbc",  BASE,     SBC,  ZP_X_IND,     0xe1, 1, false, false, false, },
  { "sbc",  BASE,     SBC,  ZP_IND_Y,     0xf1, 1, true,  false, false, },
  { "sbc",  BASE,     SBC,  ABSOLUTE,     0xed, 1, false, false, false, },
  { "sbc",  BASE,     SBC,  ABSOLUTE_X,   0xfd, 1, true,  false, false, },
  { "sbc",  BASE,     SBC,  ABSOLUTE_Y,   0xf9, 1, true,  false, false, },

  { "sec",  BASE,     SEC,  IMPLIED,      0x38, 2, false, false, false, },

  { "sed",  BASE,     SED,  IMPLIED,      0xf8, 2, false, false, false, },

  { "see",  CBM_CMOS, SEE,  IMPLIED,      0x03, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "sei",  BASE,     SEI,  IMPLIED,      0x78, 2, false, false, false, },

  { "smb0", ROCKWELL, SMB0, ZERO_PAGE,    0x87, 3, false, false, false, },
  { "smb1", ROCKWELL, SMB1, ZERO_PAGE,    0x97, 3, false, false, false, },
  { "smb2", ROCKWELL, SMB2, ZERO_PAGE,    0xa7, 3, false, false, false, },
  { "smb3", ROCKWELL, SMB3, ZERO_PAGE,    0xb7, 3, false, false, false, },
  { "smb4", ROCKWELL, SMB4, ZERO_PAGE,    0xc7, 3, false, false, false, },
  { "smb5", ROCKWELL, SMB5, ZERO_PAGE,    0xd7, 3, false, false, false, },
  { "smb6", ROCKWELL, SMB6, ZERO_PAGE,    0xe7, 3, false, false, false, },
  { "smb7", ROCKWELL, SMB7, ZERO_PAGE,    0xf7, 3, false, false, false, },

  { "sta",  BASE,     STA,  ZERO_PAGE,    0x85, 1, false, false, false, },
  { "sta",  BASE,     STA,  ZERO_PAGE_X,  0x95, 1, false, false, false, },
  { "sta",  CMOS,     STA,  ZP_IND,       0x92, 1, false, false, false, },
  { "sta",  BASE,     STA,  ZP_X_IND,     0x81, 1, false, false, false, },
  { "sta",  BASE,     STA,  ZP_IND_Y,     0x91, 2, false, false, false, },  // page cross penalty always taken
  { "sta",  BASE,     STA,  ABSOLUTE,     0x8d, 1, false, false, false, },
  { "sta",  BASE,     STA,  ABSOLUTE_X,   0x9d, 2, false, false, false, },  // page cross penalty always taken
  { "sta",  BASE,     STA,  ABSOLUTE_Y,   0x99, 2, false, false, false, },  // page cross penalty always taken
  { "sta",  CBM_CMOS, STA,  ST_VEC_IND_Y, 0x82, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "stp",  WDC_CMOS, STP,  IMPLIED,      0xdb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "stx",  BASE,     STX,  ZERO_PAGE,    0x86, 1, false, false, false, },
  { "stx",  BASE,     STX,  ZERO_PAGE_Y,  0x96, 1, false, false, false, },
  { "stx",  BASE,     STX,  ABSOLUTE,     0x8e, 1, false, false, false, },
  { "stx",  CBM_CMOS, STX,  ABSOLUTE_Y,   0x9b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "sty",  BASE,     STY,  ZERO_PAGE,    0x84, 1, false, false, false, },
  { "sty",  BASE,     STY,  ZERO_PAGE_X,  0x94, 1, false, false, false, },
  { "sty",  BASE,     STY,  ABSOLUTE,     0x8c, 1, false, false, false, },
  { "sty",  CBM_CMOS, STY,  ABSOLUTE_X,   0x8b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "stz",  CMOS,     STZ,  ZERO_PAGE,    0x64, 1, false, false, false, },
  { "stz",  CMOS,     STZ,  ZERO_PAGE_X,  0x74, 1, false, false, false, },
  { "stz",  CMOS,     STZ,  ABSOLUTE,     0x9c, 1, false, false, false, },
  { "stz",  CMOS,     STZ,  ABSOLUTE_X,   0x9e, 2, false, false, false, },  // page cross penalty always taken

  { "tab",  CBM_CMOS, TAB,  IMPLIED,      0x5b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "tax",  BASE,     TAX,  IMPLIED,      0xaa, 2, false, false, false, },

  { "tay",  BASE,     TAY,  IMPLIED,      0xa8, 2, false, false, false, },

  { "taz",  CBM_CMOS, TAZ,  IMPLIED,      0x4b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "tba",  CBM_CMOS, TBA,  IMPLIED,      0x7b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "trb",  CMOS,     TRB,  ZERO_PAGE,    0x14, 3, false, false, false, },
  { "trb",  CMOS,     TRB,  ABSOLUTE,     0x1c, 3, false, false, false, },

  { "tsb",  CMOS,     TSB,  ZERO_PAGE,    0x04, 3, false, false, false, },
  { "tsb",  CMOS,     TSB,  ABSOLUTE,     0x0c, 3, false, false, false, },

  { "tsx",  BASE,     TSX,  IMPLIED,      0xba, 2, false, false, false, },

  { "tsy",  CBM_CMOS, TSY,  IMPLIED,      0x0b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "txa",  BASE,     TXA,  IMPLIED,      0x8a, 2, false, false, false, },

  { "txs",  BASE,     TXS,  IMPLIED,      0x9a, 2, false, false, false, },

  { "tya",  BASE,     TYA,  IMPLIED,      0x98, 2, false, false, false, },

  { "tys",  CBM_CMOS, TYS,  IMPLIED,      0x2b, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "wai",  WDC_CMOS, WAI,  IMPLIED,      0xcb, UNKNOWN_CYCLE_COUNT, false, false, false, },

  { "tza",  CBM_CMOS, TZA,  IMPLIED,      0x6b, UNKNOWN_CYCLE_COUNT, false, false, false, },
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

InstructionSet::InstructionSet(const Sets& sets):
  m_sets(sets)
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

std::uint8_t InstructionSet::address_mode_added_cycles(Mode mode)
{
  return s_address_mode_added_cycles[mode];
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


const magic_enum::containers::array<InstructionSet::Mode, std::string> s_address_mode_names
{
  /* IMPLIED      */ "implied",
  /* ACCUMULATOR  */ "accum",
  /* IMMEDIATE    */ "immed",
  /* ZERO_PAGE    */ "zp",
  /* ZERO_PAGE_X  */ "zp,X",
  /* ZERO_PAGE_Y  */ "zp,Y",
  /* ZP_IND       */ "(zp)",
  /* ZP_X_IND     */ "(zp,X)",
  /* ZP_IND_Y     */ "(zp),Y",
  /* ABSOLUTE     */ "abs",
  /* ABSOLUTE_X   */ "abs,X",
  /* ABSOLUTE_Y   */ "abs,Y",
  /* ABSOLUTE_IND */ "(abs)",
  /* ABS_X_IND    */ "(abs,X)",
  /* RELATIVE     */ "relative",
  /* ZP_RELATIVE  */ "zp rel",
  /* RELATIVE_16  */ "rel 16",
  /* ST_VEC_IND_Y */ "stvindy",
};

std::ostream& InstructionSet::print_opcode_matrix(std::ostream& os,
						  bool detail)
{
  unsigned count = 0;
  os << "    ";
  for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
  {
    os << std::format("    {:02x} ", lsd);
    if (detail)
    {
      os << std::format("    ");
    }
  }
  os << "\n";
  os << "   +";
  for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
  {
    os << std::format(" ----");
    if (detail)
    {
      os << std::format("----");
    }
    os << " +";
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
	if (detail)
	{
	  os << "    ";
	}
	++count;
      }
      else
      {
	os << "     ";
	if (detail)
	{
	  os << "    ";
	}
      }
      os << " |";
    }
    os << "\n";
    if (detail)
    {
      os << "   |";
      for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
      {
	std::uint8_t opcode = (msd << 4) | lsd;
	const Info* info = m_by_opcode[opcode];
	if (info)
	{
	  os << std::format(" {:8}", s_address_mode_names[info->mode]);
	}
	else
	{
	  os << "         ";
	}
	os << " |";
      }
      os << "\n";

      os << "   |";
      for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
      {
	std::uint8_t opcode = (msd << 4) | lsd;
	const Info* info = m_by_opcode[opcode];
	if (info)
	{
	  std::uint8_t bytes = 1 + operand_size_bytes(info->mode);;
	  std::uint8_t cycles = info->base_cycle_count + address_mode_added_cycles(info->mode);
	  cycles += (m_sets.test(CMOS) ? info->cmos_extra_cycle : 0);
	  std::string footnote = "  ";
	  if ((! m_sets.test(CMOS)) && info->nmos_extra_cycle_forced)
	  {
	    cycles++;
	  }
	  else
	  {
	    footnote = (info->mode == RELATIVE) ? "**" : ((info->page_crossing_extra_cycle) ? "* " : "  ");
	  }
	  os << std::format(" {}  {}{}  ", bytes, cycles, footnote);
	}
	else
	{
	  os << "         ";
	}
	os << " |";
      }
      os << "\n";
    }

    os << "   +";
    for (std::uint8_t lsd = 0; lsd < 0x10; lsd++)
    {
      os << std::format(" ----");
      if (detail)
      {
	os << std::format("----");
      }
      os << " +";
    }
    os << "\n";
  }
  os << "\n";
  os << "*   add 1 if page boundary crossed\n";
  os << "**  add 1 if branch occurs to same page; add 2 if branch occurs to different page\n";
  os << "\n";
  os << std::format("InstructionSet:: {} opcodes\n", count);
  return os;
}

static const std::vector<InstructionSet::Mode> s_instruction_set_summary_table_column_order_nmos
{
  IMMEDIATE,
  ABSOLUTE,
  ZERO_PAGE,
  ACCUMULATOR,
  IMPLIED,
  ZP_X_IND,
  ZP_IND_Y,
  ZERO_PAGE_X,
  ABSOLUTE_X,
  ABSOLUTE_Y,
  RELATIVE,
  ABSOLUTE_IND,
  ZERO_PAGE_Y,
};


static const std::vector<InstructionSet::Mode> s_instruction_set_summary_table_column_order_cmos
{
  IMMEDIATE,
  ABSOLUTE,
  ZERO_PAGE,
  ZP_RELATIVE,    // Rockwell BBR, BBS
  ACCUMULATOR,
  IMPLIED,
  ZP_X_IND,
  ZP_IND_Y,
  ZERO_PAGE_X,
  ABSOLUTE_X,
  ABS_X_IND,      // CMOS
  ABSOLUTE_Y,
  RELATIVE,
  ABSOLUTE_IND,
  ZP_IND,         // CMOS - table column berted with ABSOLUTE_IND
  ZERO_PAGE_Y,
};

std::ostream& InstructionSet::print_summary_table(std::ostream& os)
{
  using Row = magic_enum::containers::array<Mode, const Info*>;
  std::map<std::string, Row> table;

  const std::vector<InstructionSet::Mode>* mode_table;
  if (! m_sets.test(CMOS))
  {
    mode_table = & s_instruction_set_summary_table_column_order_nmos;
  }
  else
  {
    mode_table = & s_instruction_set_summary_table_column_order_cmos;
  }

  for (const auto& info: s_main_table)
  {
    if (! m_sets.test(info.set))
    {
      continue;
    }
    if (! table.contains(info.mnemonic))
    {
      table[info.mnemonic] = {};
    }
    if (table.at(info.mnemonic)[info.mode])
    {
      throw std::logic_error(std::format("duplicate column for mnemonic \"{}\"", info.mnemonic));
    }
    table.at(info.mnemonic)[info.mode] = &info;
  }
    
  for (auto& [mnemonic, row]: table)
  {
    std::cout << mnemonic;

    for (Mode mode: *mode_table)
    {
      const Info* info = table.at(mnemonic)[mode];
      if (info)
      {
	std::uint8_t cycles = info->base_cycle_count + address_mode_added_cycles(mode);
	cycles += (m_sets.test(CMOS) ? info->cmos_extra_cycle : 0);
	std::uint8_t bytes = 1 + operand_size_bytes(mode);;
	os << std::format(" {:02x} {} {}", info->opcode, cycles, bytes);
      }
      else
      {
	os << "       ";
      }
    }

    os << "\n";
  }
  return os;
}
