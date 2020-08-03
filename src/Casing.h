#pragma once

#include "onmt/Token.h"

namespace onmt
{

  std::pair<std::string, Casing> lowercase_token(const std::string& token);
  std::string restore_token_casing(const std::string& token, Casing casing);

  char casing_to_char(Casing type);
  Casing char_to_casing(char feature);

  enum class CaseMarkupType
  {
    None,
    Modifier,
    RegionBegin,
    RegionEnd,
  };

  Casing get_casing_from_markup(const std::string& markup);
  CaseMarkupType read_case_markup(const std::string& markup);
  std::string write_case_markup(CaseMarkupType markup, Casing casing);

  struct TokenCaseMarkup
  {
    TokenCaseMarkup(CaseMarkupType prefix_, CaseMarkupType suffix_, Casing casing_)
      : prefix(prefix_)
      , suffix(suffix_)
      , casing(casing_)
    {
    }
    CaseMarkupType prefix;
    CaseMarkupType suffix;
    Casing casing;
  };

  // In "soft" mode, this function tries to minimize the number of uppercase regions by possibly
  // including case invariant characters (numbers, symbols, etc.) in uppercase regions.
  std::vector<TokenCaseMarkup> get_case_markups(const std::vector<Token>& tokens,
                                                const bool soft = true);

}
