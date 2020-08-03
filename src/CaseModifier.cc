#include "onmt/CaseModifier.h"

#include <algorithm>

#include "onmt/Tokenizer.h"
#include "onmt/unicode/Unicode.h"

namespace onmt
{

  static std::string case_markup_prefix = "mrk_case_modifier_";
  static std::string case_markup_begin_prefix = "mrk_begin_case_region_";
  static std::string case_markup_end_prefix = "mrk_end_case_region_";

  static CaseModifier::Type update_type(CaseModifier::Type current,
                                        unicode::_type_letter type,
                                        size_t letter_index)
  {
    switch (current)
    {
    case CaseModifier::Type::None:
      if (type == unicode::_letter_lower)
        return CaseModifier::Type::Lowercase;
      if (type == unicode::_letter_upper)
        return CaseModifier::Type::Capitalized;
      break;
    case CaseModifier::Type::Lowercase:
      if (type == unicode::_letter_upper)
        return CaseModifier::Type::Mixed;
      break;
    case CaseModifier::Type::Capitalized:
      if (letter_index == 1)
      {
        if (type == unicode::_letter_lower)
          return CaseModifier::Type::Capitalized;
        if (type == unicode::_letter_upper)
          return CaseModifier::Type::Uppercase;
      }
      else
      {
        if (type == unicode::_letter_upper)
          return CaseModifier::Type::Mixed;
      }
      break;
    case CaseModifier::Type::Uppercase:
      if (type == unicode::_letter_lower)
        return CaseModifier::Type::Mixed;
      break;
    default:
      break;
    }

    return current;
  }


  std::pair<std::string, char> CaseModifier::extract_case(const std::string& token)
  {
    auto pair = extract_case_type(token);
    return std::make_pair(std::move(pair.first), type_to_char(pair.second));
  }

  std::pair<std::string, CaseModifier::Type>
  CaseModifier::extract_case_type(const std::string& token)
  {
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(token, chars, code_points);

    Type current_case = Type::None;
    std::string new_token;
    new_token.reserve(chars.size());

    for (size_t i = 0, letter_index = 0; i < chars.size(); ++i)
    {
      const auto& c = chars[i];
      const auto v = code_points[i];
      unicode::_type_letter type_letter;

      if (is_letter(v, type_letter))
      {
        current_case = update_type(current_case, type_letter, letter_index++);
        if (type_letter == unicode::_letter_upper)
          new_token += unicode::cp_to_utf8(unicode::get_lower(v));
        else
          new_token += c;
      }
      else
        new_token += c;
    }

    return std::make_pair(std::move(new_token), std::move(current_case));
  }

  std::string CaseModifier::apply_case(const std::string& token, char feat)
  {
    return apply_case(token, char_to_type(feat));
  }

  std::string CaseModifier::apply_case(const std::string& token, Type case_type)
  {
    if (case_type == Type::Lowercase || case_type == Type::None)
      return token;

    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(token, chars, code_points);

    std::string new_token;
    new_token.reserve(chars.size());

    for (size_t i = 0; i < chars.size(); ++i)
    {
      unicode::code_point_t v = code_points[i];

      if (new_token.empty() || case_type == Type::Uppercase)
      {
        unicode::code_point_t upper = unicode::get_upper(v);
        if (upper)
          v = upper;
      }

      new_token += unicode::cp_to_utf8(v);
    }

    return new_token;
  }

  char CaseModifier::type_to_char(Type type)
  {
    switch (type)
    {
    case Type::Lowercase:
      return 'L';
    case Type::Uppercase:
      return 'U';
    case Type::Mixed:
      return 'M';
    case Type::Capitalized:
      return 'C';
    default:
      return 'N';
    }
  }

  CaseModifier::Type CaseModifier::char_to_type(char feature)
  {
    switch (feature)
    {
    case 'L':
      return Type::Lowercase;
    case 'U':
      return Type::Uppercase;
    case 'M':
      return Type::Mixed;
    case 'C':
      return Type::Capitalized;
    default:
      return Type::None;
    }
  }

  static bool placeholder_starts_with(const std::string& ph, const std::string& prefix)
  {
    size_t placeholder_length = (ph.length()
                                 - Tokenizer::ph_marker_open.length()
                                 - Tokenizer::ph_marker_close.length());
    return (placeholder_length == prefix.length() + 1
            && ph.compare(Tokenizer::ph_marker_open.length(), prefix.length(), prefix) == 0);
  }

  CaseModifier::Markup CaseModifier::get_case_markup(const std::string& str)
  {
    if (!Tokenizer::is_placeholder(str))
      return Markup::None;
    if (placeholder_starts_with(str, case_markup_prefix))
      return Markup::Modifier;
    if (placeholder_starts_with(str, case_markup_begin_prefix))
      return Markup::RegionBegin;
    if (placeholder_starts_with(str, case_markup_end_prefix))
      return Markup::RegionEnd;
    return Markup::None;
  }

  CaseModifier::Type CaseModifier::get_case_modifier_from_markup(const std::string& markup)
  {
    return char_to_type(markup[markup.length() - 1 - Tokenizer::ph_marker_close.length()]);
  }

  static std::string build_placeholder(const std::string& name)
  {
    return (Tokenizer::ph_marker_open + name + Tokenizer::ph_marker_close);
  }

  std::string CaseModifier::generate_case_markup(Markup markup, Type type)
  {
    const std::string* markup_prefix = nullptr;
    switch (markup)
    {
    case Markup::Modifier:
      markup_prefix = &case_markup_prefix;
      break;
    case Markup::RegionBegin:
      markup_prefix = &case_markup_begin_prefix;
      break;
    case Markup::RegionEnd:
      markup_prefix = &case_markup_end_prefix;
      break;
    default:
      return "";
    }

    return build_placeholder(*markup_prefix + type_to_char(type));
  }


  // Returns true if the token at offset can be connected to an uppercase token or
  // capital letter. This useful to avoid closing an uppercase region if intermediate
  // tokens are case invariant.
  static bool has_connected_uppercase(const std::vector<Token>& tokens, size_t offset)
  {
    for (size_t j = offset + 1; j < tokens.size(); ++j)
    {
      auto case_type = tokens[j].case_type;
      if (case_type == CaseModifier::Type::Uppercase
          || (case_type == CaseModifier::Type::Capitalized && tokens[j].unicode_length() == 1))
        return true;
      else if (case_type != CaseModifier::Type::None)
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

  std::vector<CaseModifier::TokenMarkup>
  CaseModifier::get_case_markups(const std::vector<Token>& tokens, const bool soft)
  {
    std::vector<TokenMarkup> markups;
    markups.reserve(tokens.size());
    bool in_uppercase_region = false;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      const Token& token = tokens[i];
      auto case_type = token.case_type;
      auto markup_prefix = Markup::None;
      auto markup_suffix = Markup::None;

      if (in_uppercase_region)
      {
        // In legacy mode, we end the region if the token is not uppercase or does not belong
        // to the same word before subtokenization.
        // In soft mode, we end the region on lowercase tokens or case invariant tokens that
        // are not numbers or not followed by another uppercase sequence.
        if (false
            || (!soft
                && case_type == Type::Uppercase
                && token.type == TokenType::TrailingSubword)
            || (soft
                && (case_type == Type::Uppercase
                    || (case_type == Type::Capitalized && token.unicode_length() == 1)
                    || (case_type == Type::None
                        && !Tokenizer::is_placeholder(token.surface)
                        && (has_connected_uppercase(tokens, i)
                            || numbers_only(token.surface))))))
        {
          case_type = Type::Uppercase;
        }
        else
        {
          markups.back().suffix = Markup::RegionEnd;
          in_uppercase_region = false;
          // Rewind index to treat token in the other branch.
          --i;
          continue;
        }
      }
      else
      {
        // In soft mode, we begin region on uppercase tokens or capital letter that are
        // followed by another uppercase sequence.
        if (case_type == Type::Uppercase
            || (soft
                && case_type == Type::Capitalized
                && token.unicode_length() == 1
                && has_connected_uppercase(tokens, i)))
        {
          case_type = Type::Uppercase;
          markup_prefix = Markup::RegionBegin;
          in_uppercase_region = true;
        }
        else if (case_type == Type::Capitalized)
        {
          markup_prefix = Markup::Modifier;
        }
      }

      markups.emplace_back(markup_prefix, markup_suffix, case_type);
    }

    if (in_uppercase_region)
      markups.back().suffix = Markup::RegionEnd;

    return markups;
  }

}
