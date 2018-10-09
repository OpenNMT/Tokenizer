#pragma once

#include <string>

#include "opennmttokenizer_export.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT CaseModifier
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

    static std::pair<std::string, Type> extract_case_type(const std::string& token);
    static std::pair<std::string, char> extract_case(const std::string& token);
    static std::string apply_case(const std::string& token, char feat);
    static std::string apply_case(const std::string& token, Type type);

    static char type_to_char(Type type);
    static Type char_to_type(char feature);

    enum class Markup
    {
      Modifier,
      RegionBegin,
      RegionEnd,
      None
    };

    static Markup get_case_markup(const std::string& str);
    static Type get_case_modifier_from_markup(const std::string& markup);
    static std::string generate_case_markup(Type type);
    static std::string generate_case_markup_begin(Type type);
    static std::string generate_case_markup_end(Type type);
  };

}
