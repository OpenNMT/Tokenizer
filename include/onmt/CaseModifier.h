#pragma once

#include <string>

namespace onmt
{

  class CaseModifier
  {
  public:
    enum class Type
    {
      Lowercase,
      Uppercase,
      Mixed,
      Capitalized,
      CapitalizedFirst,
      None
    };

    static std::pair<std::string, char> extract_case(const std::string& token);
    static std::string apply_case(const std::string& token, char feat);

    static char type_to_char(Type type);
    static Type char_to_type(char feature);
  };

}
