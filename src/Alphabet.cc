#include "onmt/Alphabet.h"

namespace onmt
{

  static std::vector<std::vector<UnicodeRange>> reverse_alphabet_ranges() {
    std::vector<std::vector<UnicodeRange>> v;
    v.resize(alphabet_map.size());
    for (const auto& alphabet : alphabet_ranges)
    {
      const auto& range = alphabet.first;
      const auto& id = alphabet.second;
      v[static_cast<size_t>(id)].push_back(range);
    }
    return v;
  }

  static const auto alphabet_to_ranges = reverse_alphabet_ranges();


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

  bool is_alphabet(unicode::code_point_t c, int alphabet)
  {
    if (alphabet >= 0)
    {
      for (const auto& range : alphabet_to_ranges[alphabet])
      {
        if (c >= range.first && c <= range.second)
          return true;
      }
    }

    return false;
  }

  bool is_alphabet(unicode::code_point_t c, Alphabet alphabet)
  {
    return is_alphabet(c, static_cast<int>(alphabet));
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
