#pragma once

#include <string>

#include "onmt/unicode/Unicode.h"

namespace onmt
{

  class CaseModifier
  {
  public:
    static std::pair<std::string, char> extract_case(const std::string& token);
    static std::string apply_case(const std::string& token, char feat);

  private:
    enum class Type
    {
      Lowercase,
      Uppercase,
      Mixed,
      Capitalized,
      CapitalizedFirst,
      None
    };

    static Type update_type(Type current, unicode::_type_letter type);

    static char type_to_char(Type type);
    static Type char_to_type(char feature);
  };

}
