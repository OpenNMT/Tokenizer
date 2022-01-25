#include "onmt/unicode/Unicode.h"

#include <algorithm>
#include <cstring>

#include <unicode/locid.h>
#include <unicode/uchar.h>
#include <unicode/uscript.h>

#include "../Utils.h"


namespace onmt
{
  namespace unicode
  {

    std::string cp_to_utf8(code_point_t uc)
    {
      uint8_t s[U8_MAX_LENGTH];
      int32_t offset = 0;
      UBool error = false;
      U8_APPEND(s, offset, U8_MAX_LENGTH, uc, error);
      if (error)
        return std::string();
      return std::string(reinterpret_cast<std::string::value_type*>(s), offset);
    }

    code_point_t utf8_to_cp(const unsigned char* s, unsigned int &l)
    {
      UChar32 c = -1;
      int32_t offset = 0;
      U8_NEXT(s, offset, -1, c);
      if (c < 0)
      {
        l = 0;
        c = 0;
      }
      else
        l = offset;
      return c;
    }

    std::vector<std::string> split_utf8(const std::string& str, const std::string& sep)
    {
      return split_string(str, sep);
    }

    template <typename Callback>
    static inline void character_iterator(const std::string& str, const Callback& callback)
    {
      const char* c_str = str.c_str();
      while (*c_str)
      {
        unsigned int char_size = 0;
        code_point_t code_point = utf8_to_cp(
          reinterpret_cast<const unsigned char*>(c_str), char_size);
        if (code_point == 0)  // Ignore invalid code points.
          continue;
        callback(c_str, char_size, code_point);
        c_str += char_size;
      }
    }

    void explode_utf8(const std::string& str,
                      std::vector<std::string>& chars,
                      std::vector<code_point_t>& code_points)
    {
      chars.reserve(str.length());
      code_points.reserve(str.length());

      const auto callback = [&chars, &code_points](const char* data,
                                                   unsigned int length,
                                                   code_point_t code_point) {
        code_points.push_back(code_point);
        chars.emplace_back(data, length);
      };

      character_iterator(str, callback);
    }

    void explode_utf8_with_marks(const std::string& str,
                                 std::vector<std::string>& chars,
                                 std::vector<code_point_t>* code_points_main,
                                 std::vector<std::vector<code_point_t>>* code_points_combining,
                                 const std::vector<code_point_t>* protected_chars)
    {
      chars.reserve(str.length());
      if (code_points_main)
        code_points_main->reserve(str.length());
      if (code_points_combining)
        code_points_combining->reserve(str.length());

      const auto callback = [&](const char* data,
                                unsigned int length,
                                code_point_t code_point) {
        if (chars.empty()
            || !is_mark(code_point)
            || (protected_chars
                && std::find(protected_chars->begin(),
                             protected_chars->end(),
                             code_points_main->back()) != protected_chars->end()))
        {
          if (code_points_main)
            code_points_main->emplace_back(code_point);
          if (code_points_combining)
            code_points_combining->emplace_back();
          chars.emplace_back(data, length);
        }
        else
        {
          if (code_points_combining)
            code_points_combining->back().push_back(code_point);
          chars.back().append(data, length);
        }
      };

      character_iterator(str, callback);
    }


    size_t utf8len(const std::string& str)
    {
      size_t length = 0;
      character_iterator(str, [&length](const char*, unsigned int, code_point_t) { ++length; });
      return length;
    }

    static inline CaseType get_case_type(const int8_t category)
    {
      switch (category)
      {
      case U_LOWERCASE_LETTER:
        return CaseType::Lower;
      case U_UPPERCASE_LETTER:
        return CaseType::Upper;
      default:
        return CaseType::None;
      }
    }

    static inline CharType get_char_type(const int8_t category)
    {
      switch (category)
      {
      case U_SPACE_SEPARATOR:
      case U_LINE_SEPARATOR:
      case U_PARAGRAPH_SEPARATOR:
        return CharType::Separator;

      case U_DECIMAL_DIGIT_NUMBER:
      case U_LETTER_NUMBER:
      case U_OTHER_NUMBER:
        return CharType::Number;

      case U_UPPERCASE_LETTER:
      case U_LOWERCASE_LETTER:
      case U_TITLECASE_LETTER:
      case U_MODIFIER_LETTER:
      case U_OTHER_LETTER:
        return CharType::Letter;

      case U_NON_SPACING_MARK:
      case U_ENCLOSING_MARK:
      case U_COMBINING_SPACING_MARK:
        return CharType::Mark;

      default:
        return CharType::Other;
      }
    }

    CharType get_char_type(code_point_t u)
    {
      return get_char_type(u_charType(u));
    }

    bool is_separator(code_point_t u)
    {
      return get_char_type(u) == CharType::Separator;
    }

    bool is_letter(code_point_t u)
    {
      return get_char_type(u) == CharType::Letter;
    }

    bool is_number(code_point_t u)
    {
      return get_char_type(u) == CharType::Number;
    }

    bool is_mark(code_point_t u)
    {
      return get_char_type(u) == CharType::Mark;
    }

    CaseType get_case_v2(code_point_t u)
    {
      return get_case_type(u_charType(u));
    }

    code_point_t get_lower(code_point_t u)
    {
      return u_tolower(u);
    }

    code_point_t get_upper(code_point_t u)
    {
      return u_toupper(u);
    }

    std::vector<CharInfo> get_characters_info(const std::string& str)
    {
      std::vector<CharInfo> chars;
      chars.reserve(str.size());

      character_iterator(
        str,
        [&chars](const char* data, unsigned int length, code_point_t code_point)
        {
          const auto category = u_charType(code_point);
          const auto char_type = get_char_type(category);
          const auto case_type = get_case_type(category);
          chars.emplace_back(data, length, code_point, char_type, case_type);
        });

      return chars;
    }

    bool support_language_rules()
    {
      return U_ICU_VERSION_MAJOR_NUM >= 60;
    }

    bool is_valid_language(const char* language)
    {
      const icu::Locale locale(language);
      const char* iso3_lang = locale.getISO3Language();
      return iso3_lang[0] != '\0';
    }

    // The functions below are made backward compatible with the Kangxi and Kanbun script names
    // that were previously declared in Alphabet.h but are not Unicode script aliases.
    static const std::vector<std::pair<std::pair<const char*, int>,
                                       std::pair<code_point_t, code_point_t>>>
    compat_scripts = {
      {{"Kangxi", USCRIPT_CODE_LIMIT + 0}, {0x2F00, 0x2FD5}},
      {{"Kanbun", USCRIPT_CODE_LIMIT + 1}, {0x3190, 0x319F}},
    };

    int get_script_code(const char* script_name)
    {
      for (const auto& pair : compat_scripts)
      {
        const auto& script_info = pair.first;
        if (strcmp(script_name, script_info.first) == 0)
          return script_info.second;
      }

      return u_getPropertyValueEnum(UCHAR_SCRIPT, script_name);
    }

    const char* get_script_name(int script_code)
    {
      for (const auto& pair : compat_scripts)
      {
        const auto& script_info = pair.first;
        if (script_info.second == script_code)
          return script_info.first;
      }

      return uscript_getName(static_cast<UScriptCode>(script_code));
    }

    int get_script(code_point_t c, int previous_script)
    {
      for (const auto& pair : compat_scripts)
      {
        const auto& range = pair.second;
        if (c >= range.first && c <= range.second)
          return pair.first.second;
      }

      UErrorCode error = U_ZERO_ERROR;
      UScriptCode code = uscript_getScript(c, &error);

      switch (code)
      {
      case USCRIPT_INHERITED:
        return previous_script;
      case USCRIPT_COMMON:
      {
        // For common characters, we return previous_script if it is included in
        // their script extensions.
        UScriptCode extensions[USCRIPT_CODE_LIMIT];
        int num_extensions = uscript_getScriptExtensions(c, extensions, USCRIPT_CODE_LIMIT, &error);
        for (int i = 0; i < num_extensions; ++i)
        {
          if (extensions[i] == previous_script)
            return previous_script;
        }
        return extensions[0];
      }
      default:
        return code;
      }
    }

  }
}
