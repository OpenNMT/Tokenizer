#include "onmt/Alphabet.h"

namespace onmt
{

  bool alphabet_is_supported(const std::string& alphabet)
  {
    return alphabet_map.count(alphabet) > 0;
  }

  Alphabet alphabet_to_id(const std::string& alphabet)
  {
    return alphabet_map.at(alphabet);
  }

  const std::string& id_to_alphabet(Alphabet alphabet)
  {
    return supported_alphabets[static_cast<size_t>(alphabet)];
  }

  int get_alphabet_id(unicode::code_point_t c)
  {
    // Use binary search.
    size_t lower_bound = 0;
    size_t upper_bound = alphabet_ranges.size() - 1;

    while (lower_bound < upper_bound) {
      size_t pivot = (lower_bound + upper_bound) / 2;

      const auto& alphabet = alphabet_ranges[pivot];
      const auto& range = alphabet.first;
      const auto& id = alphabet.second;

      if (c < range.first)
        upper_bound = pivot - 1;
      else if (c > range.second)
        lower_bound = pivot + 1;
      else
        return static_cast<int>(id);
    }

    if (alphabet_ranges[lower_bound].first.first <= c
        && c <= alphabet_ranges[lower_bound].first.second)
      return static_cast<int>(alphabet_ranges[lower_bound].second);

    return -1;
  }

  const std::string& get_alphabet(unicode::code_point_t c)
  {
    static const std::string empty_str = "";

    int id = get_alphabet_id(c);
    if (id < 0)
      return empty_str;
    else
      return supported_alphabets[id];
  }

}
