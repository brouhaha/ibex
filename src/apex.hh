// apex.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_HH
#define APEX_HH

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

#include "cpu6502.hh"
#include "memory.hh"

class ApexCharacterDevice
{
public:
  virtual bool open_for_input(CPU6502Registers& registers) = 0;
  virtual bool open_for_output(CPU6502Registers& registers) = 0;
  virtual bool input_byte(CPU6502Registers& registers) = 0;
  virtual bool output_byte(CPU6502Registers& registers) = 0;
  virtual bool close(CPU6502Registers& registers) = 0;
protected:
  ApexCharacterDevice();
};
using ApexCharacterDeviceSP = std::shared_ptr<ApexCharacterDevice>;

class ApexNullDevice: public ApexCharacterDevice
{
public:
  static std::shared_ptr<ApexNullDevice> create();
  bool open_for_input(CPU6502Registers& registers) override;
  bool open_for_output(CPU6502Registers& registers) override;
  bool input_byte(CPU6502Registers& registers) override;
  bool output_byte(CPU6502Registers& registers) override;
  bool close(CPU6502Registers& registers) override;
protected:
  ApexNullDevice();
};
using ApexNullDeviceSP = std::shared_ptr<ApexNullDevice>;


class Apex
{
public:
  static std::shared_ptr<Apex> create(MemorySP memory_sp);

  void init();  // set system page values in preparation to run user program

  void vector_exec(CPU6502Registers& registers);  // emulate an Apex system call

  void handler_exec(CPU6502Registers& registers); // emulate an Apex I/O handler

  static constexpr std::size_t PAGE_SIZE = 0x100;
  static constexpr Memory::Address SYS_PAGE_ADDRESS = 0xbf00;
  static constexpr std::size_t SYS_PAGE_PROGRAM_AREA_SIZE = 0x50;

  static constexpr std::uint8_t EOF_CHARACTER = 0x1a;  // AKA control-Z, ASCII SUB

  enum SysPageOffsets: Memory::Address
  {
    // offsets 0x00 through 0x4f belong to the program

    VRSTRT = 0x00,  // 3 (JMP)  program restart vector
    VSTART = 0x03,  // 3 (JMP)  program start vector
    VEXIT  = 0x06,  // 3 (JMP   program normal exit address, usually KRENTR
    VERROR = 0x09,  // 3 (JMP)  program error exit address, usually KRELOD
    VABORT = 0x0c,  // 3 (JMP)  user abort exit address, usually KSAVER

    USRMEM = 0x15,  // 2        base addr of user program
    PROSIZ = 0x17,  // 1        user pgram size in 256-byte pages

    RERUNF = 0x20,  // 1        rerun flag
    DEXTO  = 0x21,  // 3        default extension for output files
    DESTI  = 0x24,  // 3        default extension for input files
    DEFAUL = 0x27,  // 1        single bit default flags
    SYBOMB = 0x28,  // 1        $ff if prog bombs system
    USRTOP = 0x29,  // 1        last page+1 for user program (max $b0)

    OTBUFD = 0x36,  // 2        base of output buffer
    OTBUFE = 0x38,  // 2        end of output buffer
    INBUFD = 0x3a,  // 2        base of input buffer
    INBUFE = 0x3c,  // 2        end of input buffer

    // offsets 0x50 through 0xff belong to Apex

    SYSENF = 0x50,  // 1        flag showing re-entry condition
    DEVMSK = 0x51,  // 1        mask showing valid units
    SYSDEV = 0x52,  // 1        unit system is on
    SYSBLK = 0x53,  // 2        block system file is on
    SWPBLK = 0x55,  // 2        block swap file is in
    SYSDAT = 0x57,  // 3        system date
    LINIDX = 0x5a,  // 2        input line pointer ($ff = null)
    NOWDEV = 0x5c,  // 1        current byte I/O device
    EXECUT = 0x5d,  // 1        zero if exec mode is on
    LOWER  = 0x5e,  // 1        lower case switch (0 = upper)

    ERRDEV = 0x5f,  // 1        error device number
    ERRNUM = 0x60,  // 1        device handler error number
    LINPTR = 0x61,  // 1        "real" input line pointer of handler ($ff = null)
                    //          I2L uses this but calls it LINIDX
    SAVBLK = 0x62,  // 2        disk driver aux word
    LOKMSK = 0x64,  // 1        disk driver locked units mask
    CONHOR = 0x65,  //          console horizontal width, characters per line

    // I/O information block for unit drivers
    UNIT   = 0x68,  // 1        current unit number
    BLKNO  = 0x69,  // 2        current block number
    NBLKS  = 0x6b,  // 1        number of blocks to transfer
    FADDR  = 0x6c,  // 2        address pointer
    ENDBLK = 0x6e,  // 2        auxilliary parameter

    // output file information
    OTLBLK = 0x70,  // 2        first block of output file
    OTHBLK = 0x72,  // 2        last block of output file
    OTFLG  = 0x74,  // 1        status flags
    OTNO   = 0x75,  // 1        directory number of output file
    OTDEV  = 0x76,  // 1        unit number of output file

    // input file information
    INLBLK = 0x78,  // 2        first block of input file
    INHBLK = 0x7a,  // 2        last block of input file
    INFLG  = 0x7c,  // 1        status flags
    INNO   = 0x7d,  // 1        directory number of input file
    INDEV  = 0x7e,  // 1        unit number of input file

    DRVTAB = 0xc0,  // 16       8 pointers to I/O device handlers

    // entry vectors to resident code
    KRENTR = 0xd0,  // 3 (JMP)  boot in Apex (warm start)
    KSAVER = 0xd3,  // 3 (JMP)  preserve current user image
    KRELOD = 0xd6,  // 3 (JMP)  reload Apex (cold start)
    KHAND  = 0xd9,  // 3 (JMP)  byte I/O routine
    KSCAN  = 0xdc,  // 3 (JMP)  file lookup routine
    KRESTD = 0xdf,  // 3 (JMP)  reset disk driver
    KREAD  = 0xe2,  // 3 (JMP)  read contiguous disk blocks
    KWRITE = 0xe5,  // 3 (JMP)  write contiguous disk blocks

    KSSPND = 0xfd,  // 3        suspend
  };

  static constexpr Memory::Address VECTOR_START = SYS_PAGE_ADDRESS + SysPageOffsets::KRENTR;
  static constexpr Memory::Address VECTOR_END   = SYS_PAGE_ADDRESS + SysPageOffsets::KWRITE + 3;

  static constexpr unsigned MAX_CHAR_DEVICE = 8;

  void install_character_device(unsigned device_number, ApexCharacterDeviceSP);

protected:
  Apex(MemorySP memory_sp);

  void krentr(CPU6502Registers& registers);
  void ksaver(CPU6502Registers& registers);
  void krelod(CPU6502Registers& registers);
  void khand (CPU6502Registers& registers);
  void kscan (CPU6502Registers& registers);
  void krestd(CPU6502Registers& registers);
  void kread (CPU6502Registers& registers);
  void kwrite(CPU6502Registers& registers);

  MemorySP m_memory_sp;

  std::array<ApexCharacterDeviceSP, MAX_CHAR_DEVICE> m_character_devices;
};

using ApexSP = std::shared_ptr<Apex>;

#endif // APEX_HH
