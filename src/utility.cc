// utility.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <algorithm>

#include "utility.hh"

namespace utility
{
  
  char upcase_character(char c)
  {
    switch (c)
    {
      case 'a': return 'A';
      case 'b': return 'B';
      case 'c': return 'C';
      case 'd': return 'D';
      case 'e': return 'E';
      case 'f': return 'F';
      case 'g': return 'G';
      case 'h': return 'H';
      case 'i': return 'I';
      case 'j': return 'J';
      case 'k': return 'K';
      case 'l': return 'L';
      case 'm': return 'M';
      case 'n': return 'N';
      case 'o': return 'O';
      case 'p': return 'P';
      case 'q': return 'Q';
      case 'r': return 'R';
      case 's': return 'S';
      case 't': return 'T';
      case 'u': return 'U';
      case 'v': return 'V';
      case 'w': return 'W';
      case 'x': return 'X';
      case 'y': return 'Y';
      case 'z': return 'Z';
    default:
      return c;
    }
  }

  char downcase_character(char c)
  {
    switch (c)
    {
      case 'A': return 'a';
      case 'B': return 'b';
      case 'C': return 'c';
      case 'D': return 'd';
      case 'E': return 'e';
      case 'F': return 'f';
      case 'G': return 'g';
      case 'H': return 'h';
      case 'I': return 'i';
      case 'J': return 'j';
      case 'K': return 'k';
      case 'L': return 'l';
      case 'M': return 'm';
      case 'N': return 'n';
      case 'O': return 'o';
      case 'P': return 'p';
      case 'Q': return 'q';
      case 'R': return 'r';
      case 'S': return 's';
      case 'T': return 't';
      case 'U': return 'u';
      case 'V': return 'v';
      case 'W': return 'w';
      case 'X': return 'x';
      case 'Y': return 'y';
      case 'Z': return 'z';
    default:
      return c;
    }
  }

  std::string upcase_string(const std::string& s)
  {
    std::string result = s;
    std::transform(s.begin(), s.end(),
		   result.begin(),
		   [](unsigned char c){ return upcase_character(c); });
    return result;
  }

  std::string downcase_string(const std::string& s)
  {
    std::string result = s;
    std::transform(s.begin(), s.end(),
		   result.begin(),
		   [](unsigned char c){ return downcase_character(c); });
    return result;
  }

} // end namespace utility
