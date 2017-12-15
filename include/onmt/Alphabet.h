#pragma once

#include <string>

#include "onmt/unicode/Unicode.h"

namespace onmt
{

  bool alphabet_is_supported(const std::string& alphabet);
  std::string get_alphabet(unicode::code_point_t c);

}
