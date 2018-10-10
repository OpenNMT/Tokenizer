#include "onmt/CaseModifier.h"

#include "onmt/Tokenizer.h"
#include "onmt/unicode/Unicode.h"

namespace onmt
{

  static std::string case_markup_prefix = "mrk_case_modifier_";
  static std::string case_markup_begin_prefix = "mrk_begin_case_region_";
  static std::string case_markup_end_prefix = "mrk_end_case_region_";

  static CaseModifier::Type update_type(CaseModifier::Type current, unicode::_type_letter type)
  {
    switch (current)
    {
    case CaseModifier::Type::None:
      if (type == unicode::_letter_lower)
        return CaseModifier::Type::Lowercase;
      if (type == unicode::_letter_upper)
        return CaseModifier::Type::CapitalizedFirst;
      break;
    case CaseModifier::Type::Lowercase:
      if (type == unicode::_letter_upper)
        return CaseModifier::Type::Mixed;
      break;
    case CaseModifier::Type::CapitalizedFirst:
      if (type == unicode::_letter_lower)
        return CaseModifier::Type::Capitalized;
      if (type == unicode::_letter_upper)
        return CaseModifier::Type::Uppercase;
      break;
    case CaseModifier::Type::Capitalized:
      if (type == unicode::_letter_upper)
        return CaseModifier::Type::Mixed;
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

    for (size_t i = 0; i < chars.size(); ++i)
    {
      unicode::code_point_t v = code_points[i];
      unicode::_type_letter type_letter;

      if (is_letter(v, type_letter))
      {
        current_case = update_type(current_case, type_letter);
        unicode::code_point_t lower = unicode::get_lower(v);
        if (lower)
          v = lower;
      }

      new_token += unicode::cp_to_utf8(v);
    }

    return std::make_pair(new_token, current_case);
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
    case Type::CapitalizedFirst:
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

  std::string CaseModifier::generate_case_markup(CaseModifier::Type type)
  {
    return build_placeholder(case_markup_prefix + type_to_char(type));
  }

  std::string CaseModifier::generate_case_markup_begin(CaseModifier::Type type)
  {
    return build_placeholder(case_markup_begin_prefix + type_to_char(type));
  }

  std::string CaseModifier::generate_case_markup_end(CaseModifier::Type type)
  {
    return build_placeholder(case_markup_end_prefix + type_to_char(type));
  }

}
