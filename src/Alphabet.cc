#include "onmt/Alphabet.h"

namespace onmt
{

  bool alphabet_is_supported(const std::string& alphabet)
  {
    return supported_alphabets.count(alphabet) > 0;
  }

  const std::string& get_alphabet(unicode::code_point_t c)
  {
    static const std::string empty_str = "";

    // Use binary search.
    size_t lower_bound = 0;
    size_t upper_bound = alphabet_ranges.size() - 1;

    while (lower_bound < upper_bound) {
      size_t pivot = (lower_bound + upper_bound) / 2;

      const auto& alphabet = alphabet_ranges[pivot];
      const auto& range = alphabet.first;
      const auto& name = alphabet.second;

      if (c < range.first)
        upper_bound = pivot - 1;
      else if (c > range.second)
        lower_bound = pivot + 1;
      else
        return name;
    }

    if (alphabet_ranges[lower_bound].first.first <= c
        && c <= alphabet_ranges[lower_bound].first.second)
      return alphabet_ranges[lower_bound].second;

    return empty_str;
  }

}
