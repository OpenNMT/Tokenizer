#include "onmt/Alphabet.h"

#include <unordered_map>
#include <vector>

namespace onmt
{

  // WARNING: keep this vector sorted in increasing order!
  static std::vector<std::pair<std::vector<unicode::code_point_t>, std::string > > alphabet_ranges = {
    {{0x0020, 0x007F}, "Latin"},
    {{0x00A0, 0x00FF}, "Latin"},
    {{0x0100, 0x017F}, "Latin"},
    {{0x0180, 0x024F}, "Latin"},
    {{0x0370, 0x03FF}, "Greek"},
    {{0x0400, 0x04FF}, "Cyrillic"},
    {{0x0500, 0x052F}, "Cyrillic"},
    {{0x0530, 0x058F}, "Armenian"},
    {{0x0590, 0x05FF}, "Hebrew"},
    {{0x0600, 0x06FF}, "Arabic"},
    {{0x0700, 0x074F}, "Syriac"},
    {{0x0780, 0x07BF}, "Thaana"},
    {{0x0900, 0x097F}, "Devanagari"},
    {{0x0980, 0x09FF}, "Bengali"},
    {{0x0A00, 0x0A7F}, "Gurmukhi"},
    {{0x0A80, 0x0AFF}, "Gujarati"},
    {{0x0B00, 0x0B7F}, "Oriya"},
    {{0x0B80, 0x0BFF}, "Tamil"},
    {{0x0C00, 0x0C7F}, "Telugu"},
    {{0x0C80, 0x0CFF}, "Kannada"},
    {{0x0D00, 0x0D7F}, "Malayalam"},
    {{0x0D80, 0x0DFF}, "Sinhala"},
    {{0x0E00, 0x0E7F}, "Thai"},
    {{0x0E80, 0x0EFF}, "Lao"},
    {{0x0F00, 0x0FFF}, "Tibetan"},
    {{0x1000, 0x109F}, "Myanmar"},
    {{0x10A0, 0x10FF}, "Georgian"},
    {{0x1100, 0x11FF}, "Hangul"},
    {{0x1200, 0x137F}, "Ethiopic"},
    {{0x13A0, 0x13FF}, "Cherokee"},
    {{0x1680, 0x169F}, "Ogham"},
    {{0x1700, 0x171F}, "Tagalog"},
    {{0x1720, 0x173F}, "Hanunoo"},
    {{0x1740, 0x175F}, "Buhid"},
    {{0x1760, 0x177F}, "Tagbanwa"},
    {{0x1780, 0x17FF}, "Khmer"},
    {{0x1800, 0x18AF}, "Mongolian"},
    {{0x1900, 0x194F}, "Limbu"},
    {{0x2800, 0x28FF}, "Braille"},
    {{0x2E80, 0x2EFF}, "Han"},
    {{0x2F00, 0x2FDF}, "Kangxi"},
    {{0x3040, 0x309F}, "Hiragana"},
    {{0x30A0, 0x30FF}, "Katakana"},
    {{0x3100, 0x312F}, "Bopomofo"},
    {{0x3130, 0x318F}, "Hangul"},
    {{0x3190, 0x319F}, "Kanbun"},
    {{0x31A0, 0x31BF}, "Bopomofo"},
    {{0x31F0, 0x31FF}, "Katakana"},
    {{0x3200, 0x32FF}, "Han"},
    {{0x3300, 0x33FF}, "Han"},
    {{0x3400, 0x4DBF}, "Han"},
    {{0x4E00, 0x9FFF}, "Han"},
    {{0xA000, 0xA48F}, "Yi"},
    {{0xA490, 0xA4CF}, "Yi"},
    {{0xAC00, 0xD7AF}, "Hangul"},
    {{0xF900, 0xFAFF}, "Han"},
    {{0xFB50, 0xFDFF}, "Arabic"},
    {{0xFE30, 0xFE4F}, "Han"},
    {{0xFE70, 0xFEFF}, "Arabic"}};

  std::string get_alphabet(unicode::code_point_t c)
  {
    // Use binary search.
    size_t lower_bound = 0;
    size_t upper_bound = alphabet_ranges.size() - 1;

    while (lower_bound < upper_bound) {
      size_t pivot = (lower_bound + upper_bound) / 2;

      const auto& alphabet = alphabet_ranges[pivot];
      const auto& range = alphabet.first;
      const auto& name = alphabet.second;

      if (c < range[0])
        upper_bound = pivot - 1;
      else if (c > range[1])
        lower_bound = pivot + 1;
      else
        return name;
    }

    return "";
  }

}
