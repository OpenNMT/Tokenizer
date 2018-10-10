#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace onmt
{
  namespace unicode
  {

    typedef int32_t code_point_t;

    std::string cp_to_utf8(code_point_t u);
    code_point_t utf8_to_cp(const unsigned char* s, unsigned int &l);

    std::vector<std::string> split_utf8(const std::string& str, const std::string& sep);
    void explode_utf8(const std::string& str,
                      std::vector<std::string>& chars,
                      std::vector<code_point_t>& code_points);

    size_t utf8len(const std::string& str);

    enum _type_letter
    {
      _letter_other,
      _letter_lower,
      _letter_upper
    };

    bool is_separator(code_point_t u);
    bool is_letter(code_point_t u, _type_letter &tl);
    bool is_letter(code_point_t u);
    bool is_number(code_point_t u);
    bool is_mark(code_point_t u);

    _type_letter get_case(code_point_t u);
    code_point_t get_upper(code_point_t u);
    code_point_t get_lower(code_point_t u);

  }
}
