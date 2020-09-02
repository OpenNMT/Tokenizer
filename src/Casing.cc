#include "Casing.h"

#include <algorithm>

#include "onmt/Tokenizer.h"
#include "onmt/unicode/Unicode.h"
#include "Utils.h"

namespace onmt
{

  static const std::string case_markup_prefix = "mrk_case_modifier_";
  static const std::string case_markup_begin_prefix = "mrk_begin_case_region_";
  static const std::string case_markup_end_prefix = "mrk_end_case_region_";

  static inline Casing update_casing(Casing current_casing,
                                     unicode::CaseType letter_case,
                                     size_t letter_index)
  {
    switch (current_casing)
    {
    case Casing::None:
      if (letter_case == unicode::CaseType::Lower)
        return Casing::Lowercase;
      if (letter_case == unicode::CaseType::Upper)
        return Casing::Capitalized;
      break;
    case Casing::Lowercase:
      if (letter_case == unicode::CaseType::Upper)
        return Casing::Mixed;
      break;
    case Casing::Capitalized:
      if (letter_index == 1)
      {
        if (letter_case == unicode::CaseType::Lower)
          return Casing::Capitalized;
        if (letter_case == unicode::CaseType::Upper)
          return Casing::Uppercase;
      }
      else
      {
        if (letter_case == unicode::CaseType::Upper)
          return Casing::Mixed;
      }
      break;
    case Casing::Uppercase:
      if (letter_case == unicode::CaseType::Lower)
        return Casing::Mixed;
      break;
    default:
      break;
    }

    return current_casing;
  }


  std::pair<std::string, Casing> lowercase_token(const std::string& token)
  {
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(token, chars, code_points);

    Casing current_casing = Casing::None;
    std::string new_token;
    new_token.reserve(chars.size());

    for (size_t i = 0, letter_index = 0; i < chars.size(); ++i)
    {
      const auto& c = chars[i];
      const auto v = code_points[i];

      if (unicode::is_letter(v))
      {
        const unicode::CaseType letter_case = unicode::get_case_v2(v);
        current_casing = update_casing(current_casing, letter_case, letter_index++);
        if (letter_case == unicode::CaseType::Upper)
          new_token += unicode::cp_to_utf8(unicode::get_lower(v));
        else
          new_token += c;
      }
      else
        new_token += c;
    }

    return std::make_pair(std::move(new_token), std::move(current_casing));
  }

  std::string restore_token_casing(const std::string& token, Casing casing)
  {
    if (casing == Casing::Lowercase || casing == Casing::None)
      return token;

    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(token, chars, code_points);

    std::string new_token;
    new_token.reserve(chars.size());

    for (size_t i = 0; i < chars.size(); ++i)
    {
      unicode::code_point_t v = code_points[i];

      if (new_token.empty() || casing == Casing::Uppercase)
      {
        unicode::code_point_t upper = unicode::get_upper(v);
        if (upper)
          v = upper;
      }

      new_token += unicode::cp_to_utf8(v);
    }

    return new_token;
  }

  char casing_to_char(Casing casing)
  {
    switch (casing)
    {
    case Casing::Lowercase:
      return 'L';
    case Casing::Uppercase:
      return 'U';
    case Casing::Mixed:
      return 'M';
    case Casing::Capitalized:
      return 'C';
    default:
      return 'N';
    }
  }

  Casing char_to_casing(char feature)
  {
    switch (feature)
    {
    case 'L':
      return Casing::Lowercase;
    case 'U':
      return Casing::Uppercase;
    case 'M':
      return Casing::Mixed;
    case 'C':
      return Casing::Capitalized;
    default:
      return Casing::None;
    }
  }

  static inline bool placeholder_starts_with(const std::string& ph, const std::string& prefix)
  {
    size_t placeholder_length = (ph.length()
                                 - Tokenizer::ph_marker_open.length()
                                 - Tokenizer::ph_marker_close.length());
    return (placeholder_length == prefix.length() + 1
            && ph.compare(Tokenizer::ph_marker_open.length(), prefix.length(), prefix) == 0);
  }

  CaseMarkupType read_case_markup(const std::string& markup)
  {
    if (!is_placeholder(markup))
      return CaseMarkupType::None;
    if (placeholder_starts_with(markup, case_markup_prefix))
      return CaseMarkupType::Modifier;
    if (placeholder_starts_with(markup, case_markup_begin_prefix))
      return CaseMarkupType::RegionBegin;
    if (placeholder_starts_with(markup, case_markup_end_prefix))
      return CaseMarkupType::RegionEnd;
    return CaseMarkupType::None;
  }

  Casing get_casing_from_markup(const std::string& markup)
  {
    return char_to_casing(markup[markup.length() - 1 - Tokenizer::ph_marker_close.length()]);
  }

  static inline std::string build_placeholder(const std::string& name)
  {
    return (Tokenizer::ph_marker_open + name + Tokenizer::ph_marker_close);
  }

  std::string write_case_markup(CaseMarkupType markup, Casing casing)
  {
    const std::string* markup_prefix = nullptr;
    switch (markup)
    {
    case CaseMarkupType::Modifier:
      markup_prefix = &case_markup_prefix;
      break;
    case CaseMarkupType::RegionBegin:
      markup_prefix = &case_markup_begin_prefix;
      break;
    case CaseMarkupType::RegionEnd:
      markup_prefix = &case_markup_end_prefix;
      break;
    default:
      return "";
    }

    return build_placeholder(*markup_prefix + casing_to_char(casing));
  }


  // Returns true if the token at offset can be connected to an uppercase token or
  // capital letter. This useful to avoid closing an uppercase region if intermediate
  // tokens are case invariant.
  static bool has_connected_uppercase(const std::vector<Token>& tokens, size_t offset)
  {
    for (size_t j = offset + 1; j < tokens.size(); ++j)
    {
      const Token& token = tokens[j];
      if (token.casing == Casing::Uppercase
          || (token.casing == Casing::Capitalized && token.unicode_length() == 1))
        return true;
      else if (token.casing != Casing::None)
        break;
    }
    return false;
  }

  // Returns true if token only contains numbers.
  static bool numbers_only(const std::string& token)
  {
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;
    unicode::explode_utf8(token, chars, code_points);
    return std::all_of(code_points.begin(), code_points.end(), unicode::is_number);
  }

  std::vector<TokenCaseMarkup> get_case_markups(const std::vector<Token>& tokens,
                                                const bool soft)
  {
    std::vector<TokenCaseMarkup> markups;
    markups.reserve(tokens.size());
    bool in_uppercase_region = false;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      const Token& token = tokens[i];
      auto casing = token.casing;
      auto markup_prefix = CaseMarkupType::None;
      auto markup_suffix = CaseMarkupType::None;

      if (in_uppercase_region)
      {
        // In legacy mode, we end the region if the token is not uppercase or does not belong
        // to the same word before subtokenization.
        // In soft mode, we end the region on lowercase tokens or case invariant tokens that
        // are not numbers or not followed by another uppercase sequence.
        if (false
            || (!soft
                && casing == Casing::Uppercase
                && token.type == TokenType::TrailingSubword)
            || (soft
                && (casing == Casing::Uppercase
                    || (casing == Casing::Capitalized && token.unicode_length() == 1)
                    || (casing == Casing::None
                        && !token.is_placeholder()
                        && (has_connected_uppercase(tokens, i)
                            || numbers_only(token.surface))))))
        {
          casing = Casing::Uppercase;
        }
        else
        {
          markups.back().suffix = CaseMarkupType::RegionEnd;
          in_uppercase_region = false;
          // Rewind index to treat token in the other branch.
          --i;
          continue;
        }
      }
      else
      {
        // In soft mode, we begin the region on uppercase tokens or capital letter that are
        // followed by another uppercase sequence.
        if (casing == Casing::Uppercase
            || (soft
                && casing == Casing::Capitalized
                && token.unicode_length() == 1
                && has_connected_uppercase(tokens, i)))
        {
          casing = Casing::Uppercase;
          markup_prefix = CaseMarkupType::RegionBegin;
          in_uppercase_region = true;
        }
        else if (casing == Casing::Capitalized)
        {
          markup_prefix = CaseMarkupType::Modifier;
        }
      }

      markups.emplace_back(markup_prefix, markup_suffix, casing);
    }

    if (in_uppercase_region)
      markups.back().suffix = CaseMarkupType::RegionEnd;

    return markups;
  }

}
