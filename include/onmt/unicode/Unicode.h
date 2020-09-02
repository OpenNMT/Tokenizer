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

    OPENNMTTOKENIZER_EXPORT void explode_utf8(const std::string& str,
                                              std::vector<std::string>& chars,
                                              std::vector<code_point_t>& code_points);

    OPENNMTTOKENIZER_EXPORT void
    explode_utf8_with_marks(const std::string& str,
                            std::vector<std::string>& chars,
                            std::vector<code_point_t>* code_points_main = nullptr,
                            std::vector<std::vector<code_point_t>>* code_points_combining = nullptr,
                            const std::vector<std::string>* protected_chars = nullptr);

    OPENNMTTOKENIZER_EXPORT size_t utf8len(const std::string& str);

    OPENNMTTOKENIZER_EXPORT bool is_separator(code_point_t u);
    OPENNMTTOKENIZER_EXPORT bool is_letter(code_point_t u);
    OPENNMTTOKENIZER_EXPORT bool is_number(code_point_t u);
    OPENNMTTOKENIZER_EXPORT bool is_mark(code_point_t u);

    enum class CaseType {
      Lower,
      Upper,
      None,
    };

    OPENNMTTOKENIZER_EXPORT CaseType get_case_v2(code_point_t u);
    OPENNMTTOKENIZER_EXPORT code_point_t get_upper(code_point_t u);
    OPENNMTTOKENIZER_EXPORT code_point_t get_lower(code_point_t u);

    OPENNMTTOKENIZER_EXPORT int get_script_code(const char* script_name);
    OPENNMTTOKENIZER_EXPORT const char* get_script_name(int script_code);
    OPENNMTTOKENIZER_EXPORT int get_script(code_point_t c);


    // The symbols below are deprecated but kept for backward compatibility.

    OPENNMTTOKENIZER_EXPORT std::vector<std::string> split_utf8(const std::string& str,
                                                                const std::string& sep);

    inline void explode_utf8_with_marks(const std::string& str,
                                        std::vector<std::string>& chars,
                                        std::vector<code_point_t>& code_points_main,
                                        std::vector<std::vector<code_point_t>>& code_points_combining,
                                        bool keep_code_points=true)
    {
      if (!keep_code_points)
        explode_utf8_with_marks(str, chars);
      else
        explode_utf8_with_marks(str, chars, &code_points_main, &code_points_combining);
    }

    enum _type_letter
    {
      _letter_other,
      _letter_lower,
      _letter_upper
    };

    inline _type_letter get_case(code_point_t u)
    {
      switch (get_case_v2(u))
      {
      case CaseType::Lower:
        return _type_letter::_letter_lower;
      case CaseType::Upper:
        return _type_letter::_letter_upper;
      default:
        return _type_letter::_letter_other;
      }
    }

    inline bool is_letter(code_point_t u, _type_letter &tl)
    {
      if (is_letter(u))
      {
        tl = get_case(u);
        return true;
      }
      return false;
    }

  }
}
