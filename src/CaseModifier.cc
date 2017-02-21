#include "onmt/CaseModifier.h"

#include "onmt/unicode/Unicode.h"

namespace onmt
{

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
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(token, chars, code_points);

    Type current_case = Type::None;
    std::string new_token;

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

    return std::make_pair(new_token, type_to_char(current_case));
  }

  std::string CaseModifier::apply_case(const std::string& token, char feat)
  {
    Type case_type = char_to_type(feat);

    if (case_type == Type::Lowercase || case_type == Type::None)
      return token;

    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(token, chars, code_points);

    std::string new_token;

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

}
