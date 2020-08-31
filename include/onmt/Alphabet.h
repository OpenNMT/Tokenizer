#pragma once

#include <unordered_map>

#include "onmt/unicode/Unicode.h"

namespace onmt
{

  enum class Alphabet
  {
    Arabic = 0,
    Armenian,
    Bengali,
    Bopomofo,
    Braille,
    Buhid,
    Cherokee,
    Cyrillic,
    Devanagari,
    Ethiopic,
    Georgian,
    Greek,
    Gujarati,
    Gurmukhi,
    Han,
    Hangul,
    Hanunoo,
    Hebrew,
    Hiragana,
    Kanbun,
    Kangxi,
    Kannada,
    Katakana,
    Khmer,
    Lao,
    Latin,
    Limbu,
    Malayalam,
    Mongolian,
    Myanmar,
    Ogham,
    Oriya,
    Sinhala,
    Syriac,
    Tagalog,
    Tagbanwa,
    Tamil,
    Telugu,
    Thaana,
    Thai,
    Tibetan,
    Yi
  };

  const std::vector<std::string> supported_alphabets =
  {
    "Arabic",
    "Armenian",
    "Bengali",
    "Bopomofo",
    "Braille",
    "Buhid",
    "Cherokee",
    "Cyrillic",
    "Devanagari",
    "Ethiopic",
    "Georgian",
    "Greek",
    "Gujarati",
    "Gurmukhi",
    "Han",
    "Hangul",
    "Hanunoo",
    "Hebrew",
    "Hiragana",
    "Kanbun",
    "Kangxi",
    "Kannada",
    "Katakana",
    "Khmer",
    "Lao",
    "Latin",
    "Limbu",
    "Malayalam",
    "Mongolian",
    "Myanmar",
    "Ogham",
    "Oriya",
    "Sinhala",
    "Syriac",
    "Tagalog",
    "Tagbanwa",
    "Tamil",
    "Telugu",
    "Thaana",
    "Thai",
    "Tibetan",
    "Yi"
  };

  const std::unordered_map<std::string, Alphabet> alphabet_map = {
    {"Arabic", Alphabet::Arabic},
    {"Armenian", Alphabet::Armenian},
    {"Bengali", Alphabet::Bengali},
    {"Bopomofo", Alphabet::Bopomofo},
    {"Braille", Alphabet::Braille},
    {"Buhid", Alphabet::Buhid},
    {"Cherokee", Alphabet::Cherokee},
    {"Cyrillic", Alphabet::Cyrillic},
    {"Devanagari", Alphabet::Devanagari},
    {"Ethiopic", Alphabet::Ethiopic},
    {"Georgian", Alphabet::Georgian},
    {"Greek", Alphabet::Greek},
    {"Gujarati", Alphabet::Gujarati},
    {"Gurmukhi", Alphabet::Gurmukhi},
    {"Han", Alphabet::Han},
    {"Hangul", Alphabet::Hangul},
    {"Hanunoo", Alphabet::Hanunoo},
    {"Hebrew", Alphabet::Hebrew},
    {"Hiragana", Alphabet::Hiragana},
    {"Kanbun", Alphabet::Kanbun},
    {"Kangxi", Alphabet::Kangxi},
    {"Kannada", Alphabet::Kannada},
    {"Katakana", Alphabet::Katakana},
    {"Khmer", Alphabet::Khmer},
    {"Lao", Alphabet::Lao},
    {"Latin", Alphabet::Latin},
    {"Limbu", Alphabet::Limbu},
    {"Malayalam", Alphabet::Malayalam},
    {"Mongolian", Alphabet::Mongolian},
    {"Myanmar", Alphabet::Myanmar},
    {"Ogham", Alphabet::Ogham},
    {"Oriya", Alphabet::Oriya},
    {"Sinhala", Alphabet::Sinhala},
    {"Syriac", Alphabet::Syriac},
    {"Tagalog", Alphabet::Tagalog},
    {"Tagbanwa", Alphabet::Tagbanwa},
    {"Tamil", Alphabet::Tamil},
    {"Telugu", Alphabet::Telugu},
    {"Thaana", Alphabet::Thaana},
    {"Thai", Alphabet::Thai},
    {"Tibetan", Alphabet::Tibetan},
    {"Yi", Alphabet::Yi}
  };

  bool alphabet_is_supported(const std::string& alphabet);

  Alphabet alphabet_to_id(const std::string& alphabet);
  const std::string& id_to_alphabet(Alphabet alphabet);

  const std::string& get_alphabet(unicode::code_point_t c);
  int get_alphabet_id(unicode::code_point_t c);

  bool is_alphabet(unicode::code_point_t c, int alphabet);
  bool is_alphabet(unicode::code_point_t c, Alphabet alphabet);

  using UnicodeRange = std::pair<unicode::code_point_t, unicode::code_point_t>;

  // WARNING: keep this vector sorted in increasing order!
  const std::vector<std::pair<UnicodeRange, Alphabet>> alphabet_ranges = {
    {{0x0020, 0x007F}, Alphabet::Latin},
    {{0x00A0, 0x00FF}, Alphabet::Latin},
    {{0x0100, 0x017F}, Alphabet::Latin},
    {{0x0180, 0x024F}, Alphabet::Latin},
    {{0x0370, 0x03FF}, Alphabet::Greek},
    {{0x0400, 0x04FF}, Alphabet::Cyrillic},
    {{0x0500, 0x052F}, Alphabet::Cyrillic},
    {{0x0530, 0x058F}, Alphabet::Armenian},
    {{0x0590, 0x05FF}, Alphabet::Hebrew},
    {{0x0600, 0x06FF}, Alphabet::Arabic},
    {{0x0700, 0x074F}, Alphabet::Syriac},
    {{0x0780, 0x07BF}, Alphabet::Thaana},
    {{0x0900, 0x097F}, Alphabet::Devanagari},
    {{0x0980, 0x09FF}, Alphabet::Bengali},
    {{0x0A00, 0x0A7F}, Alphabet::Gurmukhi},
    {{0x0A80, 0x0AFF}, Alphabet::Gujarati},
    {{0x0B00, 0x0B7F}, Alphabet::Oriya},
    {{0x0B80, 0x0BFF}, Alphabet::Tamil},
    {{0x0C00, 0x0C7F}, Alphabet::Telugu},
    {{0x0C80, 0x0CFF}, Alphabet::Kannada},
    {{0x0D00, 0x0D7F}, Alphabet::Malayalam},
    {{0x0D80, 0x0DFF}, Alphabet::Sinhala},
    {{0x0E00, 0x0E7F}, Alphabet::Thai},
    {{0x0E80, 0x0EFF}, Alphabet::Lao},
    {{0x0F00, 0x0FFF}, Alphabet::Tibetan},
    {{0x1000, 0x109F}, Alphabet::Myanmar},
    {{0x10A0, 0x10FF}, Alphabet::Georgian},
    {{0x1100, 0x11FF}, Alphabet::Hangul},
    {{0x1200, 0x137F}, Alphabet::Ethiopic},
    {{0x13A0, 0x13FF}, Alphabet::Cherokee},
    {{0x1680, 0x169F}, Alphabet::Ogham},
    {{0x1700, 0x171F}, Alphabet::Tagalog},
    {{0x1720, 0x173F}, Alphabet::Hanunoo},
    {{0x1740, 0x175F}, Alphabet::Buhid},
    {{0x1760, 0x177F}, Alphabet::Tagbanwa},
    {{0x1780, 0x17FF}, Alphabet::Khmer},
    {{0x1800, 0x18AF}, Alphabet::Mongolian},
    {{0x1900, 0x194F}, Alphabet::Limbu},
    {{0x2800, 0x28FF}, Alphabet::Braille},
    {{0x2E80, 0x2EFF}, Alphabet::Han},
    {{0x2F00, 0x2FDF}, Alphabet::Kangxi},
    {{0x3040, 0x309F}, Alphabet::Hiragana},
    {{0x30A0, 0x30FF}, Alphabet::Katakana},
    {{0x3100, 0x312F}, Alphabet::Bopomofo},
    {{0x3130, 0x318F}, Alphabet::Hangul},
    {{0x3190, 0x319F}, Alphabet::Kanbun},
    {{0x31A0, 0x31BF}, Alphabet::Bopomofo},
    {{0x31F0, 0x31FF}, Alphabet::Katakana},
    {{0x3200, 0x32FF}, Alphabet::Han},
    {{0x3300, 0x33FF}, Alphabet::Han},
    {{0x3400, 0x4DBF}, Alphabet::Han},
    {{0x4E00, 0x9FFF}, Alphabet::Han},
    {{0xA000, 0xA48F}, Alphabet::Yi},
    {{0xA490, 0xA4CF}, Alphabet::Yi},
    {{0xAC00, 0xD7AF}, Alphabet::Hangul},
    {{0xF900, 0xFAFF}, Alphabet::Han},
    {{0xFB50, 0xFDFF}, Alphabet::Arabic},
    {{0xFE30, 0xFE4F}, Alphabet::Han},
    {{0xFE70, 0xFEFF}, Alphabet::Arabic},
    {{0xFF61, 0xFF9F}, Alphabet::Katakana},
  };

}
