#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

namespace onmt
{
  namespace unicode
  {

    typedef uint32_t code_point_t;
    typedef std::map<code_point_t, std::vector<code_point_t> > map_of_list_t;
    typedef std::unordered_map<code_point_t, code_point_t> map_unicode;

    std::string cp_to_utf8(code_point_t u);
    code_point_t utf8_to_cp(const unsigned char* s, unsigned int &l);

    std::vector<std::string> split_utf8(const std::string& str, const std::string& sep);
    void explode_utf8(std::string str,
                      std::vector<std::string>& chars,
                      std::vector<code_point_t>& code_points);

    enum _type_letter
    {
      _letter_other,
      _letter_lower,
      _letter_upper
    };

    bool is_separator(code_point_t u);
    bool is_letter(code_point_t u, _type_letter &tl);
    bool is_number(code_point_t u);

    code_point_t get_upper(code_point_t u);
    code_point_t get_lower(code_point_t u);

  }
}
