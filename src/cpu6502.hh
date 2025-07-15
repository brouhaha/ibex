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
  enum class Flag
  {
    C, Z, I, D, B, P5, V, N,
  };

  bool test(Flag flag) const;
  void set(Flag flag, bool value = true);
  void clear(Flag flag);

  void set_flags_for_result(std::uint8_t result);

  Memory::Data a;
  Memory::Data x;
  Memory::Data y;
  Memory::Data s;  // stack pointer
  Memory::Data p;  // status flags
  Memory::Address pc;
};

std::ostream& operator<<(std::ostream& os, const CPU6502Registers& reg);

class CPU6502
{
public:
  static std::shared_ptr<CPU6502> create(MemorySP memory_sp);
  void set_trace(bool value);
  void execute_instruction();
  void execute_rts();

  CPU6502Registers registers;  // direct access

  static constexpr Memory::Address STACK_BASE_ADDRESS = 0x0100;

  static constexpr Memory::Address RESET_VECTOR = 0xfffc;
  static constexpr Memory::Address IRQ_VECTOR = 0xfffe;
  static constexpr Memory::Address NMI_VECTOR = 0xfffa;

private:
  CPU6502(MemorySP memory_sp);
  std::uint16_t compute_effective_address(InstructionSet::Mode mode);
  void stack_push(std::uint8_t byte);
  std::uint8_t stack_pop();
  void halt(std::uint16_t address);

  MemorySP m_memory_sp;
  InstructionSetSP m_instruction_set_sp;
  bool m_trace;

  using ExecutionFnPtr = void (CPU6502::*)(std::uint16_t ea, bool acc);
  static const magic_enum::containers::array<InstructionSet::Inst, ExecutionFnPtr> s_execute_inst_fn_ptrs;

  void execute_ADC(std::uint16_t ea, bool acc);
  void execute_AND(std::uint16_t ea, bool acc);
  void execute_ASL(std::uint16_t ea, bool acc);
  void execute_BCC(std::uint16_t ea, bool acc);
  void execute_BCS(std::uint16_t ea, bool acc);
  void execute_BEQ(std::uint16_t ea, bool acc);
  void execute_BIT(std::uint16_t ea, bool acc);
  void execute_BMI(std::uint16_t ea, bool acc);
  void execute_BNE(std::uint16_t ea, bool acc);
  void execute_BPL(std::uint16_t ea, bool acc);
  void execute_BRK(std::uint16_t ea, bool acc);
  void execute_BVC(std::uint16_t ea, bool acc);
  void execute_BVS(std::uint16_t ea, bool acc);
  void execute_CLC(std::uint16_t ea, bool acc);
  void execute_CLD(std::uint16_t ea, bool acc);
  void execute_CLI(std::uint16_t ea, bool acc);
  void execute_CLV(std::uint16_t ea, bool acc);
  void execute_CMP(std::uint16_t ea, bool acc);
  void execute_CPX(std::uint16_t ea, bool acc);
  void execute_CPY(std::uint16_t ea, bool acc);
  void execute_DEC(std::uint16_t ea, bool acc);
  void execute_DEX(std::uint16_t ea, bool acc);
  void execute_DEY(std::uint16_t ea, bool acc);
  void execute_EOR(std::uint16_t ea, bool acc);
  void execute_INC(std::uint16_t ea, bool acc);
  void execute_INX(std::uint16_t ea, bool acc);
  void execute_INY(std::uint16_t ea, bool acc);
  void execute_JMP(std::uint16_t ea, bool acc);
  void execute_JSR(std::uint16_t ea, bool acc);
  void execute_LDA(std::uint16_t ea, bool acc);
  void execute_LDX(std::uint16_t ea, bool acc);
  void execute_LDY(std::uint16_t ea, bool acc);
  void execute_LSR(std::uint16_t ea, bool acc);
  void execute_NOP(std::uint16_t ea, bool acc);
  void execute_ORA(std::uint16_t ea, bool acc);
  void execute_PHA(std::uint16_t ea, bool acc);
  void execute_PHP(std::uint16_t ea, bool acc);
  void execute_PLA(std::uint16_t ea, bool acc);
  void execute_PLP(std::uint16_t ea, bool acc);
  void execute_ROL(std::uint16_t ea, bool acc);
  void execute_ROR(std::uint16_t ea, bool acc);
  void execute_RTI(std::uint16_t ea, bool acc);
  void execute_RTS(std::uint16_t ea, bool acc);
  void execute_SBC(std::uint16_t ea, bool acc);
  void execute_SEC(std::uint16_t ea, bool acc);
  void execute_SED(std::uint16_t ea, bool acc);
  void execute_SEI(std::uint16_t ea, bool acc);
  void execute_STA(std::uint16_t ea, bool acc);
  void execute_STX(std::uint16_t ea, bool acc);
  void execute_STY(std::uint16_t ea, bool acc);
  void execute_TAX(std::uint16_t ea, bool acc);
  void execute_TAY(std::uint16_t ea, bool acc);
  void execute_TSX(std::uint16_t ea, bool acc);
  void execute_TXA(std::uint16_t ea, bool acc);
  void execute_TXS(std::uint16_t ea, bool acc);
  void execute_TYA(std::uint16_t ea, bool acc);
};

using CPU6502SP = std::shared_ptr<CPU6502>;

#endif // APEX_HH
