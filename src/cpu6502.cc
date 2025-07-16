// cpu6502.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <format>
#include <stdexcept>

#include <magic_enum.hpp>

#include "cpu6502.hh"

bool CPU6502Registers::test(Flag flag) const
{
  return (p >> magic_enum::enum_integer(flag)) & 1;
}

void CPU6502Registers::set(Flag flag, bool value)
{
  if (value)
  {
    p |= (1 << magic_enum::enum_integer(flag));
  }
  else
  {
    p &= ~(1 << magic_enum::enum_integer(flag));
  }
}

void CPU6502Registers::clear(Flag flag)
{
  p &= ~(1 << magic_enum::enum_integer(flag));
}

void CPU6502Registers::set_flags_for_result(std::uint8_t result)
{
  set(Flag::Z, result == 0);
  set(Flag::N, (result >> 7) & 1);
}

std::ostream& operator<<(std::ostream& os, const CPU6502Registers& reg)
{
  os << std::format("PC {:04x}, A {:02x}, X {:02x}, Y {:02x}, S {:02x}, P {:02x} (",
		    reg.pc, reg.a, reg.x, reg.y, reg.s, reg.p);
  for (int i = 7; i >= 0; i--)
  {
    os << ((reg.p & (1 << i)) ? "czidb5vn"[i] : '.');
  }
  os << ")";
  return os;
}


std::shared_ptr<CPU6502> CPU6502::create(MemorySP memory_sp)
{
  auto p = new CPU6502(memory_sp);
  return std::shared_ptr<CPU6502>(p);
}

CPU6502::CPU6502(MemorySP memory_sp):
  m_memory_sp(memory_sp),
  m_instruction_set_sp(InstructionSet::create()),
  m_trace(false)
{
}

void CPU6502::set_trace(bool value)
{
  m_trace = value;
}

using enum InstructionSet::Mode;

void CPU6502::execute_instruction()
{
  std::uint8_t opcode = m_memory_sp->read_8(registers.pc);
  const InstructionSet::Info* info = m_instruction_set_sp->get(opcode);
  if (! info)
  {
    throw std::runtime_error(std::format("undefined opcode {:02x}",
					 opcode));
    
  }
  if (m_trace)
  {
    std::array<std::uint8_t, 3> bytes
    {
      m_memory_sp->read_8(registers.pc),
      m_memory_sp->read_8(registers.pc + 1),
      m_memory_sp->read_8(registers.pc + 2),
    };
    if (false)
    {
      std::cout << std::format("*** {:04x} {}\n", registers.pc, m_instruction_set_sp->disassemble(registers.pc, bytes));
    }
  }
  ++registers.pc;
  std::uint16_t ea = compute_effective_address(info->mode);
  (this->*s_execute_inst_fn_ptrs[info->inst])(ea, info->mode == ACCUMULATOR);
  if (m_trace)
  {
    std::cout << "--- " << registers << "\n";
  }
}

void CPU6502::execute_rts()
{
  execute_RTS(0, 0);
  if (m_trace)
  {
    std::cout << "--- " << registers << "\n";
  }
}

std::uint16_t CPU6502::compute_effective_address(InstructionSet::Mode mode)
{
  std::uint16_t ea;
  std::uint16_t temp_addr;
  switch (mode)
  {
  case IMPLIED:
  case ACCUMULATOR:
    break;
  case IMMEDIATE:
    ea = registers.pc++;
    break;
  case ZERO_PAGE:
    ea = m_memory_sp->read_8(registers.pc++);
    break;
  case ZERO_PAGE_X:
    ea = m_memory_sp->read_8(registers.pc++);
    ea = (ea + registers.x) & 0xff;
    break;
  case ZERO_PAGE_Y:
    ea = m_memory_sp->read_8(registers.pc++);
    ea = (ea + registers.y) & 0xff;
    break;
  case ZP_X_IND:
    temp_addr = m_memory_sp->read_8(registers.pc++);
    temp_addr = (temp_addr + registers.x) & 0xff;
    ea = m_memory_sp->read_8(temp_addr);
    ea |= (m_memory_sp->read_8((temp_addr + 1) & 0xff) << 8);
    break;
  case ZP_IND_Y:
    temp_addr = m_memory_sp->read_8(registers.pc++);
    ea = m_memory_sp->read_8(temp_addr);
    ea |= (m_memory_sp->read_8((temp_addr + 1) & 0xff) << 8);
    ea += registers.y;
    break;
  case ABSOLUTE:
    ea = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    break;
  case ABSOLUTE_X:
    ea = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    ea += registers.x;
    break;
  case ABSOLUTE_Y:
    ea = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    ea += registers.y;
    break;
  case ABSOLUTE_IND:
    temp_addr = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    // NMOS 6502 only increments low byte
    ea = m_memory_sp->read_8(temp_addr);
    temp_addr = (temp_addr & 0xff00) | ((temp_addr + 1) & 0x00ff);
    ea |= (m_memory_sp->read_8(temp_addr) << 8);
    break;
  case RELATIVE:
    ea = m_memory_sp->read_8(registers.pc++);
    if (ea & 0x80)
    {
      ea |= 0xff00;
    }
    ea += registers.pc;
    break;
  default:
    throw std::logic_error("unknown addressing mode");
  }
  return ea;
}

void CPU6502::CPU6502::stack_push(std::uint8_t byte)
{
  Memory::Address addr = STACK_BASE_ADDRESS | (registers.s--);
  m_memory_sp->write_8(addr, byte);
}

std::uint8_t CPU6502::stack_pop()
{
  Memory::Address addr = STACK_BASE_ADDRESS | (++registers.s);
  return m_memory_sp->read_8(addr);
}

void CPU6502::halt(std::uint16_t address)
{
  std::cerr << std::format("halted at instruction at {:04x}\n", address);
  std::cerr << "registers:" << registers << "\n";
  std::exit(3);
}

void CPU6502::execute_ADC(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t operand = m_memory_sp->read_8(ea);
  bool carry_in = registers.test(CPU6502Registers::Flag::C);
  std::uint16_t result = registers.a + operand + carry_in;
  std::uint8_t result_7_bit = (registers.a & 0x7f) + (operand & 0x7f) + carry_in;
  bool carry_8 = result >> 8;
  bool carry_7 = result_7_bit >> 7;
  result &= 0xff;
  registers.set(CPU6502Registers::Flag::Z, result == 0);
  if (! registers.test(CPU6502Registers::Flag::D))
  {
    registers.set(CPU6502Registers::Flag::N, result >> 7);
    registers.set(CPU6502Registers::Flag::C, carry_8);
    registers.set(CPU6502Registers::Flag::V, carry_8 ^ carry_7);
    registers.a = result;
  }
  else
  {
    // see US patent 3,991,307, Integrated circuit microprocessor with
    // parallel binary adder having on-the-fly correction to provide
    // decimal results, Charles Ingerham Peddle et al.
    std::uint8_t result_4_bit = (registers.a & 0x0f) + (operand & 0x0f) + carry_in;
    bool carry_4 = result_4_bit >> 4;
    if (carry_4)
    {
      result = (result & 0xf0) | ((result + 0x06) & 0x0f);
    }
    else if ((result & 0x0f) > 9)
    {
      carry_8 |= (result >= 0xfa);
      result = (result + 0x06) & 0xff;
    }
    registers.set(CPU6502Registers::Flag::N, result >> 7);
    registers.set(CPU6502Registers::Flag::V, carry_8 ^ carry_7);
    carry_8 |= (result >= 0xa0);
    if (carry_8)
    {
      result = (result + 0x60) & 0xff;
    }
    registers.set(CPU6502Registers::Flag::C, carry_8);
    registers.a = result;
  }
}

void CPU6502::execute_AND(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a &= m_memory_sp->read_8(ea);
  registers.set_flags_for_result(registers.a);
}

void CPU6502::execute_ASL(std::uint16_t ea,
			  bool acc)
{
  std::uint8_t byte;
  if (acc)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea);
  }
  registers.set(CPU6502Registers::Flag::C, byte >> 7);
  byte <<= 1;
  registers.set_flags_for_result(byte);
  if (acc)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea, byte);
  }
}

void CPU6502::execute_BCC(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (! registers.test(CPU6502Registers::Flag::C))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BCS(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (registers.test(CPU6502Registers::Flag::C))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BEQ(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (registers.test(CPU6502Registers::Flag::Z))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BIT(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t operand = m_memory_sp->read_8(ea);
  std::uint8_t result = operand & registers.a;
  registers.set(CPU6502Registers::Flag::Z, result == 0);
  registers.set(CPU6502Registers::Flag::N, (operand >> 7) & 1);
  registers.set(CPU6502Registers::Flag::V, (operand >> 6) & 1);
}

void CPU6502::execute_BMI(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (registers.test(CPU6502Registers::Flag::N))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BNE(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (! registers.test(CPU6502Registers::Flag::Z))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BPL(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (! registers.test(CPU6502Registers::Flag::N))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BRK([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  ++registers.pc;
  stack_push(registers.pc >> 8);
  stack_push(registers.pc & 0xff);
  stack_push(registers.p | 0x30);  // break bit, reserved bit
  registers.set(CPU6502Registers::Flag::I);
  registers.pc = m_memory_sp->read_8(IRQ_VECTOR);
  registers.pc |= (m_memory_sp->read_8(IRQ_VECTOR + 1) << 8);
}

void CPU6502::execute_BVC(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (! registers.test(CPU6502Registers::Flag::V))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_BVS(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (registers.test(CPU6502Registers::Flag::V))
  {
    if (ea == (registers.pc - 2))
    {
      halt(registers.pc - 2);
    }
    registers.pc = ea;
  }
}

void CPU6502::execute_CLC([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.clear(CPU6502Registers::Flag::C);
}

void CPU6502::execute_CLD([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.clear(CPU6502Registers::Flag::D);
}

void CPU6502::execute_CLI([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.clear(CPU6502Registers::Flag::I);
}

void CPU6502::execute_CLV([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.clear(CPU6502Registers::Flag::V);
}

void CPU6502::execute_CMP(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t operand = m_memory_sp->read_8(ea);
  operand ^= 0xff;
  std::uint16_t result = registers.a + operand + 1;
  bool carry = (result >> 8) & 1;
  result &= 0xff;
  registers.set_flags_for_result(result);
  registers.set(CPU6502Registers::Flag::C, carry);
}

void CPU6502::execute_CPX(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t operand = m_memory_sp->read_8(ea) ^ 0xff;
  std::uint16_t result = registers.x + operand + 1;
  bool carry = (result >> 8) & 1;
  result &= 0xff;
  registers.set_flags_for_result(result);
  registers.set(CPU6502Registers::Flag::C, carry);
}

void CPU6502::execute_CPY(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t operand = m_memory_sp->read_8(ea) ^ 0xff;
  std::uint16_t result = registers.y + operand + 1;
  bool carry = (result >> 8) & 1;
  result &= 0xff;
  registers.set_flags_for_result(result);
  registers.set(CPU6502Registers::Flag::C, carry);
}

void CPU6502::execute_DEC(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t byte = m_memory_sp->read_8(ea);
  byte--;
  m_memory_sp->write_8(ea, byte);
  registers.set_flags_for_result(byte);
}

void CPU6502::execute_DEX([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.x -= 1;
  registers.set_flags_for_result(registers.x);
}

void CPU6502::execute_DEY([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.y -= 1;
  registers.set_flags_for_result(registers.y);
}

void CPU6502::execute_EOR(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a ^= m_memory_sp->read_8(ea);
  registers.set_flags_for_result(registers.a);
}

void CPU6502::execute_INC(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t byte = m_memory_sp->read_8(ea);
  byte++;
  m_memory_sp->write_8(ea, byte);
  registers.set_flags_for_result(byte);
}

void CPU6502::execute_INX([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.x += 1;
  registers.set_flags_for_result(registers.x);
}

void CPU6502::execute_INY([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.y += 1;
  registers.set_flags_for_result(registers.y);
}

void CPU6502::execute_JMP(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  if (ea == (registers.pc - 3))
  {
    halt(registers.pc - 3);
  }
  registers.pc = ea;
}

void CPU6502::execute_JSR(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint16_t ret_addr = registers.pc - 1;
  stack_push(ret_addr >> 8);
  stack_push(ret_addr & 0xff);
  registers.pc = ea;
}

void CPU6502::execute_LDA(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a = m_memory_sp->read_8(ea);
  registers.set_flags_for_result(registers.a);
}

void CPU6502::execute_LDX(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.x = m_memory_sp->read_8(ea);
  registers.set_flags_for_result(registers.x);
}

void CPU6502::execute_LDY(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.y = m_memory_sp->read_8(ea);
  registers.set_flags_for_result(registers.y);
}

void CPU6502::execute_LSR(std::uint16_t ea,
			  bool acc)
{
  std::uint8_t byte;
  if (acc)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea);
  }
  registers.set(CPU6502Registers::Flag::C, byte & 1);
  byte >>= 1;
  registers.set_flags_for_result(byte);
  if (acc)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea, byte);
  }
}

void CPU6502::execute_NOP([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
}

void CPU6502::execute_ORA(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a |= m_memory_sp->read_8(ea);
  registers.set_flags_for_result(registers.a);
}

void CPU6502::execute_PHA([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  stack_push(registers.a);
}

void CPU6502::execute_PHP([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  stack_push(registers.p | 0x30);  // break set, reserved bit set
}

void CPU6502::execute_PLA([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a = stack_pop();
  registers.set_flags_for_result(registers.a);
}

void CPU6502::execute_PLP([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.p = stack_pop() | 0x20;  // undefined bit always 1
}

void CPU6502::execute_ROL(std::uint16_t ea,
			  bool acc)
{
  std::uint8_t byte;
  if (acc)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea);
  }
  bool new_carry = byte >> 7;
  byte <<= 1;
  byte |= registers.test(CPU6502Registers::Flag::C);
  registers.set(CPU6502Registers::Flag::C, new_carry);
  registers.set_flags_for_result(byte);
  if (acc)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea, byte);
  }
}

void CPU6502::execute_ROR(std::uint16_t ea,
			  bool acc)
{
  std::uint8_t byte;
  if (acc)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea);
  }
  bool new_carry = byte & 1;
  byte >>= 1;
  byte |= (registers.test(CPU6502Registers::Flag::C) << 7);
  registers.set(CPU6502Registers::Flag::C, new_carry);
  registers.set_flags_for_result(byte);
  if (acc)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea, byte);
  }
}

void CPU6502::execute_RTI([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.p = stack_pop();
  std::uint16_t addr = stack_pop();
  addr |= (stack_pop() << 8);
  registers.pc = addr;
}

void CPU6502::execute_RTS([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint16_t addr = stack_pop();
  addr |= (stack_pop() << 8);
  addr++;
  registers.pc = addr;
}

void CPU6502::execute_SBC(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  std::uint8_t operand = m_memory_sp->read_8(ea) ^ 0xff;
  bool carry_in = registers.test(CPU6502Registers::Flag::C);
  std::uint16_t result = registers.a + operand + carry_in;
  std::uint8_t result_7_bit = (registers.a & 0x7f) + (operand & 0x7f) + carry_in;
  bool carry_8 = (result >> 8) & 1;
  bool carry_7 = (result_7_bit >> 7) & 1;
  result &= 0xff;
  registers.set_flags_for_result(result);
  registers.set(CPU6502Registers::Flag::C, carry_8);
  registers.set(CPU6502Registers::Flag::V, carry_8 ^ carry_7);
  if (! registers.test(CPU6502Registers::Flag::D))
  {
    registers.a = result;
  }
  else
  {
    // see decimal mode comments in execute_ADC()
    std::uint8_t result_4_bit = (registers.a & 0x0f) + (operand & 0x0f) + carry_in;
    bool carry_4 = result_4_bit >> 4;
    if (! carry_4)
    {
      result = (result & 0xf0) | ((result + 0xfa) & 0xf);
      // for 65C02, just result = (result + 0xfa) & 0xf;
    }
    if (! carry_8)
    {
      result = (result + 0xa0) & 0xff;
    }
    registers.a = result;
  }
}

void CPU6502::execute_SEC([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.set(CPU6502Registers::Flag::C);
}

void CPU6502::execute_SED([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.set(CPU6502Registers::Flag::D);
}

void CPU6502::execute_SEI([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.set(CPU6502Registers::Flag::I);
}

void CPU6502::execute_STA(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  m_memory_sp->write_8(ea, registers.a);
}

void CPU6502::execute_STX(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  m_memory_sp->write_8(ea, registers.x);
}

void CPU6502::execute_STY(std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  m_memory_sp->write_8(ea, registers.y);
}

void CPU6502::execute_TAX([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.x = registers.a;
  registers.set_flags_for_result(registers.x);
}

void CPU6502::execute_TAY([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.y = registers.a;
  registers.set_flags_for_result(registers.y);
}

void CPU6502::execute_TSX([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.x = registers.s;
  registers.set_flags_for_result(registers.x);
}

void CPU6502::execute_TXA([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a = registers.x;
  registers.set_flags_for_result(registers.a);
}

void CPU6502::execute_TXS([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.s = registers.x;
}

void CPU6502::execute_TYA([[maybe_unused]] std::uint16_t ea,
			  [[maybe_unused]] bool acc)
{
  registers.a = registers.y;
  registers.set_flags_for_result(registers.a);
}

const magic_enum::containers::array<InstructionSet::Inst, CPU6502::ExecutionFnPtr> CPU6502::s_execute_inst_fn_ptrs
{
  & CPU6502::execute_ADC,
  & CPU6502::execute_AND,
  & CPU6502::execute_ASL,
  & CPU6502::execute_BCC,
  & CPU6502::execute_BCS,
  & CPU6502::execute_BEQ,
  & CPU6502::execute_BIT,
  & CPU6502::execute_BMI,
  & CPU6502::execute_BNE,
  & CPU6502::execute_BPL,
  & CPU6502::execute_BRK,
  & CPU6502::execute_BVC,
  & CPU6502::execute_BVS,
  & CPU6502::execute_CLC,
  & CPU6502::execute_CLD,
  & CPU6502::execute_CLI,
  & CPU6502::execute_CLV,
  & CPU6502::execute_CMP,
  & CPU6502::execute_CPX,
  & CPU6502::execute_CPY,
  & CPU6502::execute_DEC,
  & CPU6502::execute_DEX,
  & CPU6502::execute_DEY,
  & CPU6502::execute_EOR,
  & CPU6502::execute_INC,
  & CPU6502::execute_INX,
  & CPU6502::execute_INY,
  & CPU6502::execute_JMP,
  & CPU6502::execute_JSR,
  & CPU6502::execute_LDA,
  & CPU6502::execute_LDX,
  & CPU6502::execute_LDY,
  & CPU6502::execute_LSR,
  & CPU6502::execute_NOP,
  & CPU6502::execute_ORA,
  & CPU6502::execute_PHA,
  & CPU6502::execute_PHP,
  & CPU6502::execute_PLA,
  & CPU6502::execute_PLP,
  & CPU6502::execute_ROL,
  & CPU6502::execute_ROR,
  & CPU6502::execute_RTI,
  & CPU6502::execute_RTS,
  & CPU6502::execute_SBC,
  & CPU6502::execute_SEC,
  & CPU6502::execute_SED,
  & CPU6502::execute_SEI,
  & CPU6502::execute_STA,
  & CPU6502::execute_STX,
  & CPU6502::execute_STY,
  & CPU6502::execute_TAX,
  & CPU6502::execute_TAY,
  & CPU6502::execute_TSX,
  & CPU6502::execute_TXA,
  & CPU6502::execute_TXS,
  & CPU6502::execute_TYA,
};
