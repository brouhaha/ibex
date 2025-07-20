// cpu6502.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CPU6502_HH
#define CPU6502_HH

#include <iostream>
#include <memory>

#include "instruction_set.hh"
#include "memory.hh"

class CPU6502Registers
{
public:
  //                                          6502     65802
  //                                          65C02    65816    65CE02    init
  //                                          -----    -----    ------    -----
  std::uint16_t   a; // accumulator             8       8/16       8

  std::uint16_t   x; // index reg               8       8/16       8        
  std::uint16_t   y; // index_reg                       8/16       8

  std::uint16_t  pc; // program counter        16       16        16
  std::uint16_t   s; // stack pointer           8       16        16      $01xx

  std::uint16_t   d; // direct, base page      n/a      16      8 (hi)    $0000

  std::uint8_t    p; // status (flags)          8        8         8

  std::uint8_t  dbr; // data bank              n/a       8       n/a       $00
  std::uint8_t  pbr; // program bank           n/a       8       n/a       $00

  std::uint8_t    z; // index reg/zero         n/a      n/a        8       $00

  bool            e; // emulation mode         n/a       1       n/a        1

  // status register (flag) definitions and operations
  enum class Flag
  {
    C = 0,
    Z = 1,
    I = 2,
    D = 3,

    X = 4, B = 4,  // 65802/65816 only, always 1 on others,
                   //                   except 0 when pushed by IRQ (vs. BRK)

    M = 5, P5 = 5, // 65802/65816 only, always 1 on others
    V = 6,
    N = 7,
  };

  CPU6502Registers();

  bool test(Flag flag) const;
  void set(Flag flag, bool value = true);
  void clear(Flag flag);

  bool test_c() const;
  bool test_z() const;
  bool test_i() const;
  bool test_d() const;
  bool test_x() const;
  bool test_m() const;
  bool test_v() const;
  bool test_n() const;

  void set_c(bool value);
  void set_z(bool value);
  void set_i(bool value);
  void set_d(bool value);
  void set_v(bool value);
  void set_n(bool value);

  void set_n_z_for_result(std::uint8_t result);
};

std::ostream& operator<<(std::ostream& os, const CPU6502Registers& reg);

class CPU6502
{
public:
  static std::shared_ptr<CPU6502> create(const InstructionSet::Sets& sets,
					 MemorySP memory_sp);

  void reset_counters();

  std::uint64_t get_instruction_count();
  std::uint64_t get_cycle_count();

  void set_trace(bool value);

  // returns false unless halts (tight loop branch or jump),
  // or tries to execute an undefined opcode
  bool execute_instruction();

  void execute_rts();

  CPU6502Registers registers;  // direct access

  static constexpr Memory::Address STACK_BASE_ADDRESS = 0x0100;

  enum Vector
  {
    VECTOR_NMI   = 0xfffa,
    VECTOR_RESET = 0xfffc,
    VECTOR_IRQ   = 0xfffe,

    // vectors for 65802/65816 only:
    VECTOR_COP   = 0xfff4,  // 65816 only
    VECTOR_ABORT = 0xfff8,  // 65816 only

    VECTOR_NATIVE_COP   = 0xffe4,
    VECTOR_NATIVE_BRK   = 0xffe6,
    VECTOR_NATIVE_ABORT = 0xffe8,
    VECTOR_NATIVE_NMI   = 0xffea,
    VECTOR_NATIVE_IRQ   = 0xffee,
  };

private:
  MemorySP m_memory_sp;
  InstructionSetSP m_instruction_set_sp;
  bool m_cmos;
  bool m_absolute_ind_fixed;
  bool m_interrupt_clears_decimal;
  bool m_bcd_cmos;
  bool m_halt;
  std::uint64_t m_instruction_count;
  std::uint64_t m_cycle_count;
  bool m_trace;
  std::uint8_t m_instruction_cycle_count;

  CPU6502(const InstructionSet::Sets& sets,
	  MemorySP memory_sp);

  void compute_effective_address(const InstructionSet::Info* info,
				 std::uint32_t& ea1,
				 std::uint32_t& ea2);

  void stack_push(std::uint8_t byte);
  std::uint8_t stack_pop();
  void halt(std::uint32_t address);
  void branch(std::uint32_t address);
  void go_vector(Vector vector, bool brk = false);

  using ExecutionFnPtr = void (CPU6502::*)(const InstructionSet::Info* info,
					   std::uint32_t ea1,
					   std::uint32_t ea2);
  static const magic_enum::containers::array<InstructionSet::Inst, ExecutionFnPtr> s_execute_inst_fn_ptrs;

  void execute_ADC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_AND(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_ASL(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BBR(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BBS(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BCC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BCS(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BEQ(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BIT(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BMI(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BNE(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BPL(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BRA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BRK(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BVC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_BVS(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CLC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CLD(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CLI(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CLV(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CMP(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CPX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_CPY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_DEC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_DEX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_DEY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_EOR(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_INC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_INX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_INY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_JMP(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_JSR(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_LDA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_LDX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_LDY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_LSR(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_NOP(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_ORA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PHA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PHP(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PHX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PHY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PLA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PLP(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PLX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_PLY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_RMB(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_ROL(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_ROR(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_RTI(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_RTS(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_SBC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_SEC(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_SED(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_SEI(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_SMB(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_STA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_STP(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_STX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_STY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_STZ(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TAX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TAY(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TRB(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TSB(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TSX(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TXA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TXS(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
  void execute_TYA(const InstructionSet::Info* info, std::uint32_t ea1, std::uint32_t ea2);
};

using CPU6502SP = std::shared_ptr<CPU6502>;

#endif // APEX_HH
