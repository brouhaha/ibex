// utility.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef UTILITY_HH
#define UTILITY_HH

#include <string>

namespace utility
{
  // upcase and downcase character (only alters Basic Latin letters (A-Z)
  // not dependent on locale
  // does not depend on host character set being ASCII or Unicode
  char upcase_character(char c);
  char downcase_character(char c);

  // return a upcased or downcased copy of a string (only alters Basic Latin letters (A-Z))
  // not dependent on locale
  // does not depend on host character set being ASCII or Unicode
  std::string upcase_string(const std::string& s);
  std::string downcase_string(const std::string& s);

} // end namespace utility

#endif // UTILITY_HH
