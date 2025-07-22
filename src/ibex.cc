// ibex.cc
//
// Copyright 2022-2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <format>
#include <initializer_list>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include "apex.hh"
#include "apex_console_device.hh"
#include "apex_printer_device.hh"
#include "apex_file_byte_device.hh"
#include "app_metadata.hh"
#include "cpu6502.hh"
#include "elapsed_time.hh"
#include "instruction_set.hh"
#include "memory.hh"


namespace po = boost::program_options;

void conflicting_options(const boost::program_options::variables_map& vm,
			 std::initializer_list<const std::string> opts)
{
  if (opts.size() < 2)
  {
    throw std::invalid_argument("conflicting_options requires at least two options");
  }
  for (auto opt1 = opts.begin(); opt1 < opts.end(); opt1++)
  {
    if (vm.count(*opt1))
    {
      for (auto opt2 = opt1 + 1; opt2 != opts.end(); opt2++)
      {
	if (vm.count(*opt2))
	{
	  std::cerr << std::format("Options {} and {} are mutually exclusive\n", *opt1, *opt2);
	  std::exit(1);
	}
      }
    }
  }
}


enum class ExecutableFormat
{
  APEX_SAV,
  APEX_BIN,
  RAW_BINARY,
};


static bool stats_requested = false;
static std::string memory_dump_fn;
static MemorySP memory_sp;
static CPU6502SP cpu_sp;

static ElapsedTime elapsed_time;


std::ostream& stats(std::ostream& os)
{
  double elapsed_time_seconds = elapsed_time.get_elapsed_time_seconds();

  os << "elapsed time (s): " << elapsed_time_seconds << "\n";

  std::uint64_t instruction_count = cpu_sp->get_instruction_count();
  os << std::format("{} instructions executed\n", instruction_count);
  os << std::format("{} instructions executed per second\n", instruction_count / elapsed_time_seconds);
  
  std::uint64_t cycle_count = cpu_sp->get_cycle_count();
  os << std::format("{} cycles executed\n", cycle_count);
  os << std::format("{} cycles executed per second/s\n", cycle_count / elapsed_time_seconds);

  os << std::format("average clocks per instruction: {}\n", ((double) cycle_count) / ((double) instruction_count));

  return os;
}

void finish()
{
  elapsed_time.stop();

  if (memory_dump_fn.size())
  {
    memory_sp->dump_raw_bin(memory_dump_fn);
  }
  if (stats_requested)
  {
    stats(std::cerr);
  }
}


void signal_handler(int signum)
{
  finish();
  std::exit(signum);
}


int main(int argc, char *argv[])
{
  std::cerr << std::format("{}-{}-{}\n", name, app_version_string, release_type_string);

  InstructionSet::Sets instruction_sets;
  bool cmos;

  bool trace = false;
  bool mem_trace = false;

  bool print_opcode_matrix = false;
  bool print_detail = false;
  bool print_summary_table = false;

  ExecutableFormat executable_format = ExecutableFormat::APEX_SAV;
  std::string executable_fn;
  std::uint16_t load_address = 0x0000;       // XXX need a command line argument
  std::uint16_t execution_address = 0x0400;  // XXX need a command line argument

  bool open_input_file = false;
  std::string input_fn;

  bool open_output_file = false;
  std::string output_fn;

  bool open_printer_file = false;
  std::string printer_fn;

  try
  {
    po::options_description gen_opts("Options");
    gen_opts.add_options()
      ("help",                                           "output help message")
      ("cmos,c",                                         "CMOS R65C02")
      ("bin,b",                                          "executable is in BIN format")
      ("raw,r",                                          "executable is a raw binary file")
      ("input,i",   po::value<std::string>(&input_fn),   "input file")
      ("output,o",  po::value<std::string>(&output_fn),  "output file")
      ("printer,p", po::value<std::string>(&printer_fn), "printer file")
      ("stats,s",                                        "print statistics");

    po::options_description hidden_opts("Hidden options:");
    hidden_opts.add_options()
      ("executable", po::value<std::string>(&executable_fn),  "executable filename")
      ("trace,t",                                             "trace execution")
      ("memtrace",                                            "trace memory")
      ("dump",       po::value<std::string>(&memory_dump_fn), "memory dump filename")
      ("hextable",                                            "print hex table")
      ("hextabledetail",                                      "print hex table with detail")
      ("summarytable",                                        "print summary table");

    po::positional_options_description positional_opts;
    positional_opts.add("executable", -1);

    po::options_description cmdline_opts;
    cmdline_opts.add(gen_opts).add(hidden_opts);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
	      options(cmdline_opts).positional(positional_opts).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      std::cerr << "Usage: " << argv[0] << " [options]\n\n";
      std::cerr << gen_opts << "\n";
      std::exit(0);
    }

    cmos = vm.count("cmos") != 0;
    instruction_sets = cmos ? InstructionSet::CPU_R65C02 : InstructionSet::CPU_6502;

    stats_requested = vm.count("stats");

    if (vm.count("hextable"))
    {
      print_opcode_matrix = true;
    }
    if (vm.count("hextabledetail"))
    {
      print_opcode_matrix = true;
      print_detail = true;
    }
    if (vm.count("summarytable"))
    {
      print_summary_table = true;
    }

    if (vm.count("executable") != 1)
    {
      std::cerr << "executable file must be specified\n";
      std::exit(1);
    }

    trace = vm.count("trace");
    mem_trace = vm.count("memtrace");

    if (vm.count("raw"))
    {
      executable_format = ExecutableFormat::RAW_BINARY;
    }
    else if (vm.count("bin"))
    {
      executable_format = ExecutableFormat::APEX_BIN;
    }

    open_input_file = vm.count("input");
    open_output_file = vm.count("output");
    open_printer_file = vm.count("printer");
  }
  catch (po::error& e)
  {
    std::cerr << "argument error: " << e.what() << "\n";
    std::exit(1);
  }

  if (print_opcode_matrix)
  {
    auto inst_set_sp = InstructionSet::create(instruction_sets);
    inst_set_sp->print_opcode_matrix(std::cout, print_detail);
    std::cout << "\n\n";
  }
  if (print_summary_table)
  {
    auto inst_set_sp = InstructionSet::create(instruction_sets);
    inst_set_sp->print_summary_table(std::cout);
    std::cout << "\n\n";
  }

  memory_sp = Memory::create(1ull<<16);
  cpu_sp = CPU6502::create(instruction_sets,
			   memory_sp);

  ApexSP apex_sp = Apex::create(memory_sp);

  auto null_device_sp = ApexNullDevice::create();
  apex_sp->install_character_device(7, null_device_sp);

  auto console_device_sp = ApexConsoleDevice::create();
  apex_sp->install_character_device(0, console_device_sp);
  apex_sp->install_character_device(1, console_device_sp);

  auto printer_device_sp = ApexPrinterDevice::create();
  if (open_printer_file)
  {
    printer_device_sp->open_output_file(printer_fn);
  }
  apex_sp->install_character_device(2, printer_device_sp);

  auto file_byte_device_sp = ApexFileByteDevice::create();
  if (open_input_file)
  {
    file_byte_device_sp->open_input_file(input_fn);
  }
  if (open_output_file)
  {
    file_byte_device_sp->open_output_file(output_fn);
  }
  apex_sp->install_character_device(3, file_byte_device_sp);

  cpu_sp->registers.clear(CPU6502Registers::Flag::D);

  apex_sp->init();
  switch (executable_format)
  {
  case ExecutableFormat::APEX_BIN:
    memory_sp->load_apex_bin(executable_fn);
    cpu_sp->registers.pc = Apex::SYS_PAGE_ADDRESS + Apex::SysPageOffsets::VSTART;
    break;
  case ExecutableFormat::APEX_SAV:
    memory_sp->load_apex_sav(executable_fn);
    cpu_sp->registers.pc = Apex::SYS_PAGE_ADDRESS + Apex::SysPageOffsets::VSTART;
    break;
  case ExecutableFormat::RAW_BINARY:
    memory_sp->load_raw_bin(executable_fn, load_address);
    cpu_sp->registers.pc = execution_address;
    break;
  }

  cpu_sp->registers.a = 0x00;
  cpu_sp->registers.x = 0x00;
  cpu_sp->registers.y = 0x00;
  cpu_sp->registers.s = 0xff;
  cpu_sp->registers.p = 0x34;

  cpu_sp->set_trace(trace);
  memory_sp->set_trace(mem_trace);

  std::signal(SIGINT, signal_handler);

  elapsed_time.start();

  bool run = true;
  while (run)
  {
    bool halt;
    if ((cpu_sp->registers.pc >= Apex::VECTOR_START) &&
	(cpu_sp->registers.pc < Apex::VECTOR_END))
    {
      halt = apex_sp->vector_exec(cpu_sp->registers);
      cpu_sp->execute_rts();
      if (halt)
      {
	std::cerr << "apex halt\n";
      }
    }
    else
    {
      halt = cpu_sp->execute_instruction();
      if (halt)
      {
	std::cerr << "cpu halt\n";
      }
    }
    run &= ! halt;
  }

  finish();
  return 0;
}
