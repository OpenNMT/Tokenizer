#include "Casing.h"

#include <algorithm>

#include <unicode/uvernum.h>
#if U_ICU_VERSION_MAJOR_NUM >= 60
#  include <unicode/locid.h>
#  include <unicode/unistr.h>
#  include <unicode/stringoptions.h>
#endif

#include "onmt/Tokenizer.h"
#include "Utils.h"

namespace onmt
{

  static const std::string case_markup_prefix = "mrk_case_modifier_";
  static const std::string case_markup_begin_prefix = "mrk_begin_case_region_";
  static const std::string case_markup_end_prefix = "mrk_end_case_region_";

  Casing update_casing(Casing current_casing,
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


  std::pair<std::string, Casing> lowercase_token(const std::string& token, const std::string& lang)
  {
    Casing current_casing = Casing::None;
    size_t letter_index = 0;
    std::string new_token;

#if U_ICU_VERSION_MAJOR_NUM >= 60
    if (!lang.empty())
    {
      // First resolve the casing of the token.
      for (const auto& c : unicode::get_characters_info(token))
      {
        if (c.char_type == unicode::CharType::Letter)
          current_casing = update_casing(current_casing, c.case_type, letter_index++);
      }

      // Then apply language specific lowercasing with ICU.
      icu::Locale locale(lang.c_str());
      icu::UnicodeString::fromUTF8(token).toLower(locale).toUTF8String(new_token);
      return std::make_pair(std::move(new_token), std::move(current_casing));
    }
#else
    (void)lang;
#endif

    new_token.reserve(token.size());

    for (const auto& c : unicode::get_characters_info(token))
    {
      if (c.char_type == unicode::CharType::Letter)
      {
        current_casing = update_casing(current_casing, c.case_type, letter_index++);
        if (c.case_type == unicode::CaseType::Upper)
          new_token.append(unicode::cp_to_utf8(unicode::get_lower(c.value)));
        else
          new_token.append(c.data, c.length);
      }
      else
        new_token.append(c.data, c.length);
    }

    return std::make_pair(std::move(new_token), std::move(current_casing));
  }

  std::string restore_token_casing(const std::string& token, Casing casing, const std::string& lang)
  {
    if (token.empty() || casing == Casing::Lowercase || casing == Casing::None)
      return token;
    if (casing == Casing::Mixed)
      throw std::invalid_argument("Can't restore mixed casing");

    std::string new_token;

#if U_ICU_VERSION_MAJOR_NUM >= 60
    if (!lang.empty())
    {
      // Apply language specific recasing with ICU.
      icu::Locale locale(lang.c_str());
      auto utoken = icu::UnicodeString::fromUTF8(token);
      if (casing == Casing::Capitalized)
        utoken.toTitle(nullptr, locale, U_TITLECASE_WHOLE_STRING);
      else
        utoken.toUpper(locale);
      utoken.toUTF8String(new_token);
      return new_token;
    }
#else
    (void)lang;
#endif

    new_token.reserve(token.size());

    for (const auto& c : unicode::get_characters_info(token))
    {
      if (new_token.empty() || casing == Casing::Uppercase)
        new_token.append(unicode::cp_to_utf8(unicode::get_upper(c.value)));
      else
        new_token.append(c.data, c.length);
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
    const auto chars = unicode::get_characters_info(token);
    return std::all_of(chars.begin(), chars.end(), [](const unicode::CharInfo& c) {
      return c.char_type == unicode::CharType::Number;
    });
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
