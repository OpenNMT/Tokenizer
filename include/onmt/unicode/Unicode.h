#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "onmt/opennmttokenizer_export.h"

namespace onmt
{
  namespace unicode
  {

    typedef int32_t code_point_t;

    OPENNMTTOKENIZER_EXPORT std::string cp_to_utf8(code_point_t u);
    OPENNMTTOKENIZER_EXPORT code_point_t utf8_to_cp(const unsigned char* s, unsigned int &l);

    OPENNMTTOKENIZER_EXPORT std::vector<std::string> split_utf8(const std::string& str, const std::string& sep);
    OPENNMTTOKENIZER_EXPORT void explode_utf8(const std::string& str,
                      std::vector<std::string>& chars,
                      std::vector<code_point_t>& code_points);
    OPENNMTTOKENIZER_EXPORT void
    explode_utf8_with_marks(const std::string& str,
                            std::vector<std::string>& chars,
                            std::vector<std::vector<code_point_t>>& code_points);

    OPENNMTTOKENIZER_EXPORT size_t utf8len(const std::string& str);

    enum _type_letter
    {
      _letter_other,
      _letter_lower,
      _letter_upper
    };

    OPENNMTTOKENIZER_EXPORT bool is_separator(code_point_t u);
    OPENNMTTOKENIZER_EXPORT bool is_letter(code_point_t u, _type_letter &tl);
    OPENNMTTOKENIZER_EXPORT bool is_letter(code_point_t u);
    OPENNMTTOKENIZER_EXPORT bool is_number(code_point_t u);
    OPENNMTTOKENIZER_EXPORT bool is_mark(code_point_t u);

    OPENNMTTOKENIZER_EXPORT _type_letter get_case(code_point_t u);
    OPENNMTTOKENIZER_EXPORT code_point_t get_upper(code_point_t u);
    OPENNMTTOKENIZER_EXPORT code_point_t get_lower(code_point_t u);

  }
}
