#include "onmt/BPE.h"

#include <fstream>
#include <limits>

#include "onmt/unicode/Unicode.h"

namespace onmt
{

  static std::vector<std::pair<std::string, std::string> >
  get_pairs(const std::vector<std::string>& chars)
  {
    std::vector<std::pair<std::string, std::string> > pairs;

    for (size_t i = 0; i < chars.size() - 1; ++i)
      pairs.emplace_back(chars[i], chars[i + 1]);

    return pairs;
  }

  const std::string BPE::end_of_word = "</w>";

  BPE::BPE(const std::string& model)
  {
    std::ifstream in(model.c_str());
    std::string line;

    int i = 0;

    while (std::getline(in, line))
    {
      size_t sep = line.find(' ');
      if (sep != std::string::npos && sep + 1 < line.size())
      {
        auto data = std::make_pair(line.substr(0, sep), line.substr(sep + 1));
        if (_codes.count(data) == 0)
          _codes[data] = i++;
      }
    }
  }

  std::vector<std::string> BPE::encode(const std::string& str) const
  {
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(str, chars, code_points);

    if (chars.size() == 1)
      return chars;

    chars.push_back(end_of_word);
    auto pairs = get_pairs(chars);

    while (true)
    {
      auto bigram = get_min_pair(pairs);

      if (bigram.first.empty())
        break;

      std::vector<std::string> new_chars;
      bool merge = false;

      for (size_t i = 0; i < chars.size(); ++i)
      {
        if (merge)
        {
          if (chars[i] == bigram.second)
          {
            new_chars.push_back(bigram.first + bigram.second);
            merge = false;
          }
          else if (chars[i] == bigram.first)
          {
            new_chars.push_back(bigram.first);
          }
          else
          {
            new_chars.push_back(bigram.first);
            new_chars.push_back(chars[i]);
            merge = false;
          }
        }
        else
        {
          if (chars[i] == bigram.first)
            merge = true;
          else
            new_chars.push_back(chars[i]);
        }
      }

      chars.swap(new_chars);
      if (chars.size() == 1)
        break;
      else
        pairs = get_pairs(chars);
    }

    if (chars.back() == end_of_word)
      chars.pop_back();
    else if (chars.back().substr(chars.back().size() - end_of_word.size()) == end_of_word)
    {
      std::string cleaned = chars.back().substr(0, chars.back().size() - end_of_word.size());
      chars.pop_back();
      chars.push_back(cleaned);
    }

    return chars;
  }

  std::pair<std::string, std::string>
  BPE::get_min_pair(const std::vector<std::pair<std::string, std::string> >& pairs) const
  {
    int min_score = std::numeric_limits<int>::max();
    std::pair<std::string, std::string> min_pair;

    for (size_t i = 0; i < pairs.size(); ++i)
    {
      auto it = _codes.find(pairs[i]);

      if (it != _codes.end())
      {
        int score = it->second;
        if (score < min_score)
        {
          min_score = score;
          min_pair = pairs[i];
        }
      }
    }

    return min_pair;
  }

}
