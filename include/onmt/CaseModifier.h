#pragma once

#include <string>
#include <vector>

#include "onmt/opennmttokenizer_export.h"

namespace onmt
{

  class Token;

  // TODO: this should not be a class.
  class OPENNMTTOKENIZER_EXPORT CaseModifier
  {
  public:
    enum class Type
    {
      None,
      Lowercase,
      Uppercase,
      Mixed,
      Capitalized,
    };

    static std::pair<std::string, Type> extract_case_type(const std::string& token);
    static std::pair<std::string, char> extract_case(const std::string& token);
    static std::string apply_case(const std::string& token, char feat);
    static std::string apply_case(const std::string& token, Type type);

    static char type_to_char(Type type);
    static Type char_to_type(char feature);

    enum class Markup
    {
      None,
      Modifier,
      RegionBegin,
      RegionEnd,
    };

    static Markup get_case_markup(const std::string& str);
    static std::string generate_case_markup(Markup markup, Type type);
    static Type get_case_modifier_from_markup(const std::string& markup);

    struct TokenMarkup
    {
      TokenMarkup(Markup prefix_, Markup suffix_, Type type_)
        : prefix(prefix_)
        , suffix(suffix_)
        , type(type_)
      {
      }
      Markup prefix;
      Markup suffix;
      Type type;
    };

    // In "soft" mode, this function tries to minimize the number of uppercase regions by possibly
    // including case invariant characters (numbers, symbols, etc.) in uppercase regions.
    static std::vector<TokenMarkup>
    get_case_markups(const std::vector<Token>& tokens, const bool soft = true);

  };

}
