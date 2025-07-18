// cpu6502.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <format>
#include <stdexcept>

#include <magic_enum.hpp>

#include "cpu6502.hh"

CPU6502Registers::CPU6502Registers()
{
  d = 0;
  dbr = 0;
  pbr = 0;
  z = 0;
  e = 1;
}

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

bool CPU6502Registers::test_c() const { return test(Flag::C); }
bool CPU6502Registers::test_z() const { return test(Flag::Z); }
bool CPU6502Registers::test_i() const { return test(Flag::I); }
bool CPU6502Registers::test_d() const { return test(Flag::D); }
bool CPU6502Registers::test_x() const { return e || test(Flag::X); }
bool CPU6502Registers::test_m() const { return e || test(Flag::M); }
bool CPU6502Registers::test_v() const { return test(Flag::V); }
bool CPU6502Registers::test_n() const { return test(Flag::N); }

void CPU6502Registers::set_c(bool value) { set(Flag::C, value); }
void CPU6502Registers::set_z(bool value) { set(Flag::Z, value); }
void CPU6502Registers::set_i(bool value) { set(Flag::I, value); }
void CPU6502Registers::set_d(bool value) { set(Flag::D, value); }
void CPU6502Registers::set_v(bool value) { set(Flag::V, value); }
void CPU6502Registers::set_n(bool value) { set(Flag::N, value); }

void CPU6502Registers::set_n_z_for_result(std::uint8_t result)
{
  set_z(result == 0);
  set_n((result >> 7) & 1);
}

std::ostream& operator<<(std::ostream& os, const CPU6502Registers& reg)
{
  os << std::format("PC {:04x}, A {:02x}, X {:02x}, Y {:02x}, S {:02x}, P {:02x} (",
		    reg.pc, reg.a, reg.x, reg.y, reg.s, reg.p);
  for (int i = 7; i >= 0; i--)
  {
    os << ((reg.p & (1 << i)) ? "czidxmvn"[i] : '.');
  }
  os << ")";
  return os;
}


std::shared_ptr<CPU6502> CPU6502::create(const InstructionSet::Sets& sets,
					 MemorySP memory_sp)
{
  auto p = new CPU6502(sets, memory_sp);
  return std::shared_ptr<CPU6502>(p);
}

CPU6502::CPU6502(const InstructionSet::Sets& sets,
		 MemorySP memory_sp):
  m_memory_sp(memory_sp),
  m_instruction_set_sp(InstructionSet::create(sets)),
  m_trace(false)
{
}

void CPU6502::set_trace(bool value)
{
  m_trace = value;
}

void CPU6502::go_vector(Vector vector, bool brk)
{
  if (vector == VECTOR_RESET)
  {
    registers.s = (registers.s - 3) & 0xff;
  }
  else
  {
    stack_push(registers.pc >> 8);
    stack_push(registers.pc & 0xff);
    std::uint8_t p_value = registers.p;
    if (registers.e)
    {
      p_value |= (1 << magic_enum::enum_integer(CPU6502Registers::Flag::P5));  // reserved bit always set
      if (brk)
      {
	p_value |= (1 << magic_enum::enum_integer(CPU6502Registers::Flag::B));
      }
      else
      {
	p_value &= ~(1 << magic_enum::enum_integer(CPU6502Registers::Flag::B));
      }
    }
    stack_push(p_value);
  }
  registers.set_i(true);
  std::uint16_t addr = magic_enum::enum_integer(vector);
  registers.pc = m_memory_sp->read_8(addr);
  registers.pc |= (m_memory_sp->read_8(addr + 1) << 8);
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
  std::uint32_t ea1;
  std::uint32_t ea2;
  compute_effective_address(info->mode, ea1, ea2);
  (this->*s_execute_inst_fn_ptrs[info->inst])(info, ea1, ea2);
  if (m_trace)
  {
    std::cout << "--- " << registers << "\n";
  }
}

void CPU6502::execute_rts()
{
  execute_RTS(nullptr, 0, 0);
  if (m_trace)
  {
    std::cout << "--- " << registers << "\n";
  }
}

void CPU6502::compute_effective_address(InstructionSet::Mode mode,
					std::uint32_t& ea1,
					std::uint32_t& ea2)
{
  std::uint32_t temp_addr;
  switch (mode)
  {
  case IMPLIED:
  case ACCUMULATOR:
    break;
  case IMMEDIATE:
    ea1 = (registers.pc++);
    break;
  case ZERO_PAGE:
    ea1 = m_memory_sp->read_8(registers.pc++);
    break;
  case ZERO_PAGE_X:
    temp_addr = m_memory_sp->read_8(registers.pc++);
    ea1 = (temp_addr + registers.x) & 0xff;
    break;
  case ZERO_PAGE_Y:
    temp_addr= m_memory_sp->read_8(registers.pc++);
    ea1 = (temp_addr + registers.y) & 0xff;
    break;
  case ZP_IND:  // CMOS
    temp_addr = m_memory_sp->read_8(registers.pc++);
    ea1 = m_memory_sp->read_8(temp_addr);
    ea1 |= (m_memory_sp->read_8((temp_addr + 1) & 0xff) << 8);
    break;
  case ZP_X_IND:
    temp_addr = m_memory_sp->read_8(registers.pc++);
    temp_addr = (temp_addr + registers.x) & 0xff;
    ea1 = m_memory_sp->read_8(temp_addr);
    ea1 |= (m_memory_sp->read_8((temp_addr + 1) & 0xff) << 8);
    break;
  case ZP_IND_Y:
    temp_addr = m_memory_sp->read_8(registers.pc++);
    ea1 = m_memory_sp->read_8(temp_addr);
    ea1 |= (m_memory_sp->read_8((temp_addr + 1) & 0xff) << 8);
    ea1 += registers.y;
    break;
  case ABSOLUTE:
    ea1 = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    break;
  case ABSOLUTE_X:
    ea1 = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    ea1 += registers.x;
    break;
  case ABSOLUTE_Y:
    ea1 = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    ea1 += registers.y;
    break;
  case ABSOLUTE_IND:
    temp_addr = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    ea1 = m_memory_sp->read_8(temp_addr);
    if (true)
    {
      // NMOS 6502 only increments low byte
      temp_addr = (temp_addr & 0xff00) | ((temp_addr + 1) & 0x00ff);
    }
    else
    {
      // CMOS 6502 increments entire address
      ++temp_addr;
    }
    ea1 |= (m_memory_sp->read_8(temp_addr) << 8);
    break;
  case ABS_X_IND:  // CMOS
    temp_addr = m_memory_sp->read_16_le(registers.pc);
    registers.pc += 2;
    temp_addr += registers.x;
    ea1 = m_memory_sp->read_16_le(temp_addr);
    break;
  case ZP_RELATIVE:  // Rockwell BBR, BBS
    ea1 = m_memory_sp->read_8(registers.pc++);
    [[fallthrough]];
  case RELATIVE:
    ea2 = m_memory_sp->read_8(registers.pc++);
    if (ea2 & 0x80)
    {
      ea2 |= 0xff00;
    }
    ea2 = (ea2 + registers.pc) & 0xffff;
    break;
  case RELATIVE_16:  // Commodore 65CE02
    ea2 = m_memory_sp->read_16_le(registers.pc++);
    ea2 = (ea2 + registers.pc) & 0xffff;
    break;
  case ST_VEC_IND_Y:  // Commodore 65CE02
    temp_addr = m_memory_sp->read_8(registers.pc++);
    temp_addr += 0x0100 + registers.s;
    ea1 = m_memory_sp->read_16_le(temp_addr);
    break;
  default:
    throw std::logic_error("unknown addressing mode");
  }
}

void CPU6502::CPU6502::stack_push(std::uint8_t byte)
{
  Memory::Address addr = STACK_BASE_ADDRESS | registers.s;
  registers.s = (registers.s - 1) & 0xff;
  m_memory_sp->write_8(addr, byte);
}

std::uint8_t CPU6502::stack_pop()
{
  registers.s = (registers.s + 1) & 0xff;
  Memory::Address addr = STACK_BASE_ADDRESS | registers.s;
  return m_memory_sp->read_8(addr);
}

void CPU6502::halt(std::uint32_t address)
{
  std::cerr << std::format("halted at instruction at {:04x}\n", address);
  std::cerr << "registers:" << registers << "\n";
  std::exit(3);
}

void CPU6502::execute_ADC([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  bool carry_in = registers.test(CPU6502Registers::Flag::C);
  std::uint16_t result = registers.a + operand + carry_in;
  std::uint8_t result_7_bit = (registers.a & 0x7f) + (operand & 0x7f) + carry_in;
  bool carry_8 = result >> 8;
  bool carry_7 = result_7_bit >> 7;
  result &= 0xff;
  registers.set_z(result == 0);
  if (! registers.test(CPU6502Registers::Flag::D))
  {
    registers.set_n(result >> 7);
    registers.set_c(carry_8);
    registers.set_v(carry_8 ^ carry_7);
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
    registers.set_n(result >> 7);
    registers.set_v(carry_8 ^ carry_7);
    carry_8 |= (result >= 0xa0);
    if (carry_8)
    {
      result = (result + 0x60) & 0xff;
    }
    registers.set_c(carry_8);
    registers.a = result;
  }
}

void CPU6502::execute_AND([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a &= m_memory_sp->read_8(ea1);
  registers.set_n_z_for_result(registers.a);
}

void CPU6502::execute_ASL(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t byte;
  bool accumulator = info->mode == InstructionSet::Mode::ACCUMULATOR;
  if (accumulator)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea1);
  }
  registers.set_c(byte >> 7);
  byte <<= 1;
  registers.set_n_z_for_result(byte);
  if (accumulator)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea1, byte);
  }
}

void CPU6502::execute_BBR(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  unsigned bit_num = (info->opcode >> 4) & 7;
  if (! ((operand >> bit_num) & 1))
  {
    registers.pc = ea2;
  }
}

void CPU6502::execute_BBS(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  unsigned bit_num = (info->opcode >> 4) & 7;
  if ((operand >> bit_num) & 1)
  {
    registers.pc = ea2;
  }
}

void CPU6502::execute_BCC([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (! registers.test(CPU6502Registers::Flag::C))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BCS([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (registers.test(CPU6502Registers::Flag::C))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BEQ([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (registers.test(CPU6502Registers::Flag::Z))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BIT([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  std::uint8_t result = operand & registers.a;
  registers.set_z(result == 0);
  registers.set_n((operand >> 7) & 1);
  registers.set_v((operand >> 6) & 1);
}

void CPU6502::execute_BMI([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (registers.test(CPU6502Registers::Flag::N))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BNE([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (! registers.test(CPU6502Registers::Flag::Z))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BPL([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (! registers.test(CPU6502Registers::Flag::N))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BRA([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  registers.pc = ea2;
}

void CPU6502::execute_BRK([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  ++registers.pc;  // BRK is a two-byte instruction
  go_vector(registers.e ? VECTOR_IRQ : VECTOR_NATIVE_BRK, true);
}

void CPU6502::execute_BVC([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (! registers.test(CPU6502Registers::Flag::V))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_BVS([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  std::uint32_t ea2)
{
  if (registers.test(CPU6502Registers::Flag::V))
  {
    if (ea2 == static_cast<std::uint32_t>(registers.pc - 2))
    {
      halt(ea2);
    }
    registers.pc = ea2;
  }
}

void CPU6502::execute_CLC([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_c(false);
}

void CPU6502::execute_CLD([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_d(false);
}

void CPU6502::execute_CLI([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_i(false);
}

void CPU6502::execute_CLV([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_v(false);
}

void CPU6502::execute_CMP([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  operand ^= 0xff;
  std::uint16_t result = registers.a + operand + 1;
  bool carry = (result >> 8) & 1;
  result &= 0xff;
  registers.set_n_z_for_result(result);
  registers.set_c(carry);
}

void CPU6502::execute_CPX([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1) ^ 0xff;
  std::uint16_t result = registers.x + operand + 1;
  bool carry = (result >> 8) & 1;
  result &= 0xff;
  registers.set_n_z_for_result(result);
  registers.set_c(carry);
}

void CPU6502::execute_CPY([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1) ^ 0xff;
  std::uint16_t result = registers.y + operand + 1;
  bool carry = (result >> 8) & 1;
  result &= 0xff;
  registers.set_n_z_for_result(result);
  registers.set_c(carry);
}

void CPU6502::execute_DEC(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t byte;
  bool accumulator = info->mode == InstructionSet::Mode::ACCUMULATOR;
  if (accumulator)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea1);
  }
  byte--;
  registers.set_n_z_for_result(byte);
  if (accumulator)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea1, byte);
  }
}

void CPU6502::execute_DEX([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.x = (registers.x - 1) & 0xff;
  registers.set_n_z_for_result(registers.x);
}

void CPU6502::execute_DEY([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.y = (registers.y - 1) & 0xff;
  registers.set_n_z_for_result(registers.y);
}

void CPU6502::execute_EOR([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a ^= m_memory_sp->read_8(ea1);
  registers.set_n_z_for_result(registers.a);
}

void CPU6502::execute_INC(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t byte;
  bool accumulator = info->mode == InstructionSet::Mode::ACCUMULATOR;
  if (accumulator)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea1);
  }
  byte++;
  registers.set_n_z_for_result(byte);
  if (accumulator)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea1, byte);
  }
}

void CPU6502::execute_INX([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.x = (registers.x + 1) & 0xff;
  registers.set_n_z_for_result(registers.x);
}

void CPU6502::execute_INY([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.y = (registers.y + 1) & 0xff;
  registers.set_n_z_for_result(registers.y);
}

void CPU6502::execute_JMP([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  if (ea1 == static_cast<std::uint32_t>(registers.pc - 3))
  {
    halt(ea1);
  }
  registers.pc = ea1;
}

void CPU6502::execute_JSR([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint16_t ret_addr = registers.pc - 1;
  stack_push(ret_addr >> 8);
  stack_push(ret_addr & 0xff);
  registers.pc = ea1;
}

void CPU6502::execute_LDA([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a = m_memory_sp->read_8(ea1);
  registers.set_n_z_for_result(registers.a);
}

void CPU6502::execute_LDX([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.x = m_memory_sp->read_8(ea1);
  registers.set_n_z_for_result(registers.x);
}

void CPU6502::execute_LDY([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.y = m_memory_sp->read_8(ea1);
  registers.set_n_z_for_result(registers.y);
}

void CPU6502::execute_LSR([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t byte;
  bool accumulator = info->mode == InstructionSet::Mode::ACCUMULATOR;
  if (accumulator)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea1);
  }
  registers.set_c(byte & 1);
  byte >>= 1;
  registers.set_n_z_for_result(byte);
  if (accumulator)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea1, byte);
  }
}

void CPU6502::execute_NOP([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
}

void CPU6502::execute_ORA([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a |= m_memory_sp->read_8(ea1);
  registers.set_n_z_for_result(registers.a);
}

void CPU6502::execute_PHA([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  stack_push(registers.a);
}

void CPU6502::execute_PHP([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  stack_push(registers.p | 0x30);  // break set, reserved bit set
}

void CPU6502::execute_PHX([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  stack_push(registers.x);
}

void CPU6502::execute_PHY([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  stack_push(registers.y);
}

void CPU6502::execute_PLA([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a = stack_pop();
  registers.set_n_z_for_result(registers.a);
}

void CPU6502::execute_PLP([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.p = stack_pop();
  if (registers.e)
  {
    registers.p |= ((1 << magic_enum::enum_integer(CPU6502Registers::Flag::B)) |
		    (1 << magic_enum::enum_integer(CPU6502Registers::Flag::P5)));
  }
}

void CPU6502::execute_PLX([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.x = stack_pop();
  registers.set_n_z_for_result(registers.x);
}

void CPU6502::execute_PLY([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.y = stack_pop();
  registers.set_n_z_for_result(registers.y);
}

void CPU6502::execute_RMB(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  unsigned bit_num = (info->opcode >> 4) & 7;
  operand &= ~(1 << bit_num);
  m_memory_sp->write_8(ea1, operand);
}

void CPU6502::execute_ROL([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t byte;
  bool accumulator = info->mode == InstructionSet::Mode::ACCUMULATOR;
  if (accumulator)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea1);
  }
  bool new_carry = byte >> 7;
  byte <<= 1;
  byte |= registers.test(CPU6502Registers::Flag::C);
  registers.set_c(new_carry);
  registers.set_n_z_for_result(byte);
  if (accumulator)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea1, byte);
  }
}

void CPU6502::execute_ROR([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t byte;
  bool accumulator = info->mode == InstructionSet::Mode::ACCUMULATOR;
  if (accumulator)
  {
    byte = registers.a;
  }
  else
  {
    byte = m_memory_sp->read_8(ea1);
  }
  bool new_carry = byte & 1;
  byte >>= 1;
  byte |= (registers.test(CPU6502Registers::Flag::C) << 7);
  registers.set_c(new_carry);
  registers.set_n_z_for_result(byte);
  if (accumulator)
  {
    registers.a = byte;
  }
  else
  {
    m_memory_sp->write_8(ea1, byte);
  }
}

void CPU6502::execute_RTI([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.p = stack_pop();
  if (registers.e)
  {
    registers.p |= ((1 << magic_enum::enum_integer(CPU6502Registers::Flag::B)) |
		    (1 << magic_enum::enum_integer(CPU6502Registers::Flag::P5)));
  }
  std::uint16_t addr = stack_pop();
  addr |= (stack_pop() << 8);
  registers.pc = addr;
}

void CPU6502::execute_RTS([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint16_t addr = stack_pop();
  addr |= (stack_pop() << 8);
  addr++;
  registers.pc = addr;
}

void CPU6502::execute_SBC([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1) ^ 0xff;
  bool carry_in = registers.test(CPU6502Registers::Flag::C);
  std::uint16_t result = registers.a + operand + carry_in;
  std::uint8_t result_7_bit = (registers.a & 0x7f) + (operand & 0x7f) + carry_in;
  bool carry_8 = (result >> 8) & 1;
  bool carry_7 = (result_7_bit >> 7) & 1;
  result &= 0xff;
  registers.set_n_z_for_result(result);
  registers.set_c(carry_8);
  registers.set_v(carry_8 ^ carry_7);
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

void CPU6502::execute_SEC([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_c(true);
}

void CPU6502::execute_SED([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_d(true);
}

void CPU6502::execute_SEI([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.set_i(true);
}

void CPU6502::execute_SMB(const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  unsigned bit_num = (info->opcode >> 4) & 7;
  operand |= (1 << bit_num);
  m_memory_sp->write_8(ea1, operand);
}

void CPU6502::execute_STA([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  m_memory_sp->write_8(ea1, registers.a);
}

void CPU6502::execute_STX([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  m_memory_sp->write_8(ea1, registers.x);
}

void CPU6502::execute_STY([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  m_memory_sp->write_8(ea1, registers.y);
}

void CPU6502::execute_STZ([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  m_memory_sp->write_8(ea1, 0x00);
}

void CPU6502::execute_TAX([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.x = registers.a;
  registers.set_n_z_for_result(registers.x);
}

void CPU6502::execute_TAY([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.y = registers.a;
  registers.set_n_z_for_result(registers.y);
}

void CPU6502::execute_TRB([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  std::uint8_t result = operand & ~registers.a;
  registers.set_z(result == 0);
  m_memory_sp->write_8(ea1, result);
}

void CPU6502::execute_TSB([[maybe_unused]] const InstructionSet::Info* info,
			  std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  std::uint8_t operand = m_memory_sp->read_8(ea1);
  std::uint8_t result = operand | registers.a;
  registers.set_z(result == 0);
  m_memory_sp->write_8(ea1, result);
}

void CPU6502::execute_TSX([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.x = registers.s;
  registers.set_n_z_for_result(registers.x);
}

void CPU6502::execute_TXA([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a = registers.x;
  registers.set_n_z_for_result(registers.a);
}

void CPU6502::execute_TXS([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.s = registers.x;
}

void CPU6502::execute_TYA([[maybe_unused]] const InstructionSet::Info* info,
			  [[maybe_unused]] std::uint32_t ea1,
			  [[maybe_unused]] std::uint32_t ea2)
{
  registers.a = registers.y;
  registers.set_n_z_for_result(registers.a);
}

const magic_enum::containers::array<InstructionSet::Inst, CPU6502::ExecutionFnPtr> CPU6502::s_execute_inst_fn_ptrs
{
  & CPU6502::execute_ADC,
  & CPU6502::execute_AND,
  & CPU6502::execute_ASL,
  nullptr, // & CPU6502::execute_ASR,
  nullptr, // & CPU6502::execute_ASW,
  nullptr, // & CPU6502::execute_AUG,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBR,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BBS,
  & CPU6502::execute_BCC,
  & CPU6502::execute_BCS,
  & CPU6502::execute_BEQ,
  & CPU6502::execute_BIT,
  & CPU6502::execute_BMI,
  & CPU6502::execute_BNE,
  & CPU6502::execute_BPL,
  & CPU6502::execute_BRA,
  & CPU6502::execute_BRK,
  nullptr, // & CPU6502::execute_BSR,
  & CPU6502::execute_BVC,
  & CPU6502::execute_BVS,
  & CPU6502::execute_CLC,
  & CPU6502::execute_CLD,
  nullptr, // & CPU6502::execute_CLE,
  & CPU6502::execute_CLI,
  & CPU6502::execute_CLV,
  & CPU6502::execute_CMP,
  & CPU6502::execute_CPX,
  & CPU6502::execute_CPY,
  nullptr, // & CPU6502::execute_CPZ,
  & CPU6502::execute_DEC,
  nullptr, // & CPU6502::execute_DEW,
  & CPU6502::execute_DEX,
  & CPU6502::execute_DEY,
  nullptr, // & CPU6502::execute_DEZ,
  & CPU6502::execute_EOR,
  & CPU6502::execute_INC,
  nullptr, // & CPU6502::execute_INW,
  & CPU6502::execute_INX,
  & CPU6502::execute_INY,
  nullptr, // & CPU6502::execute_INZ,
  & CPU6502::execute_JMP,
  & CPU6502::execute_JSR,
  & CPU6502::execute_LDA,
  & CPU6502::execute_LDX,
  & CPU6502::execute_LDY,
  nullptr, // & CPU6502::execute_LDZ,
  & CPU6502::execute_LSR,
  nullptr, // & CPU6502::execute_NEG,
  & CPU6502::execute_NOP,
  & CPU6502::execute_ORA,
  & CPU6502::execute_PHA,
  & CPU6502::execute_PHP,
  nullptr, // & CPU6502::execute_PHW,
  & CPU6502::execute_PHX,
  & CPU6502::execute_PHY,
  nullptr, // & CPU6502::execute_PHZ,
  & CPU6502::execute_PLA,
  & CPU6502::execute_PLP,
  nullptr, // & CPU6502::execute_PLW,
  & CPU6502::execute_PLX,
  & CPU6502::execute_PLY,
  nullptr, // & CPU6502::execute_PLZ,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_RMB,
  & CPU6502::execute_ROL,
  & CPU6502::execute_ROR,
  nullptr, // & CPU6502::execute_ROW,
  & CPU6502::execute_RTI,
  nullptr, // & CPU6502::execute_RTN,
  & CPU6502::execute_RTS,
  & CPU6502::execute_SBC,
  & CPU6502::execute_SEC,
  & CPU6502::execute_SED,
  nullptr, // & CPU6502::execute_SEE,
  & CPU6502::execute_SEI,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_SMB,
  & CPU6502::execute_STA,
  nullptr, // & CPU6502::execute_STP,
  & CPU6502::execute_STX,
  & CPU6502::execute_STY,
  & CPU6502::execute_STZ,
  nullptr, // & CPU6502::execute_TAB,
  & CPU6502::execute_TAX,
  & CPU6502::execute_TAY,
  nullptr, // & CPU6502::execute_TAZ,
  nullptr, // & CPU6502::execute_TBA,
  & CPU6502::execute_TRB,
  & CPU6502::execute_TSB,
  & CPU6502::execute_TSX,
  nullptr, // & CPU6502::execute_TSY,
  & CPU6502::execute_TXA,
  & CPU6502::execute_TXS,
  & CPU6502::execute_TYA,
  nullptr, // & CPU6502::execute_TYS,
  nullptr, // & CPU6502::execute_TZA,
  nullptr, // & CPU6502::execute_WAI,
};
