#include "onmt/BPE.h"
#include <iostream>
#include <fstream>
#include <limits>

#include "onmt/unicode/Unicode.h"
#include "onmt/CaseModifier.h"

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

  BPE::BPE(const std::string& model_path)
    : _end_of_word("</w>")
    , _begin_of_word("<w>")
    , _prefix(false)
    , _suffix(true)
    , _case_insensitive(false)
    , _compat_03(false)
  {
    std::ifstream in(model_path.c_str());

    if (!in.is_open())
      throw std::invalid_argument("Unable to open BPE model `" + model_path + "'");

    std::string line;

    int i = 0;

    std::getline(in, line);

    std::vector<std::string> options;
    std::string option;

    size_t sep = line.find(';');
    size_t bidx = 0;
    while (sep != std::string::npos && sep + 1 < line.size())
    {
      options.push_back(line.substr(bidx, sep-bidx));
      bidx = sep + 1;
      sep = line.find(';', bidx);
    }
    options.push_back(line.substr(bidx));

    if (options.size() == 6 && options[0] == "v3")
    {
      _compat_03 = options[1] == "ref03";
      _prefix = options[1] == "true";
      _suffix = options[2] == "true";
      _case_insensitive = options[3] == "true";
      _begin_of_word = options[4];
      _end_of_word = options[5];
    } else
      in.seekg(0);

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
    std::string str_lc = str;
    if (_case_insensitive)
    {
      str_lc = CaseModifier::extract_case(str).first;
    }

    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    unicode::explode_utf8(str_lc, chars, code_points);

    if (chars.size() == 1)
    {
      chars[0] = str;
      return chars;
    }

    if (_prefix) { chars.insert(chars.begin(), _begin_of_word); }
    if (_suffix) { chars.push_back(_end_of_word); }
    if (_compat_03) { chars[chars.size()-1] += _end_of_word; }

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

    if (_prefix)
    {
      if (chars.front() == _begin_of_word)
        chars.erase(chars.begin());
      else if (chars.front().substr(0, _begin_of_word.size()) == _begin_of_word)
      {
        std::string cleaned = chars.front().substr(_begin_of_word.size());
        chars.erase(chars.begin());
        chars.insert(chars.begin(), cleaned);
      }
    }

    if (_suffix)
    {
      if (chars.back() == _end_of_word)
        chars.pop_back();
      else if (chars.back().substr(chars.back().size() - _end_of_word.size()) == _end_of_word)
      {
        std::string cleaned = chars.back().substr(0, chars.back().size() - _end_of_word.size());
        chars.pop_back();
        chars.push_back(cleaned);
      }
    }

    if (_compat_03)
    {
      size_t l = chars[chars.size()-1].size();
      if (l > _end_of_word.size() &&
          chars.back().substr(l-_end_of_word.size()) == _end_of_word)
        chars[chars.size()-1].erase(l-_end_of_word.size());
    }

    if (_case_insensitive)
    {
      std::vector<std::string> word_tc;

      std::vector<std::string> chars_tc;
      std::vector<unicode::code_point_t> code_points_tc;

      unicode::explode_utf8(str, chars_tc, code_points_tc);

      std::vector<std::string>::iterator it = chars_tc.begin();
      for (size_t i = 0; i < chars.size(); ++i)
      {
        size_t curr_length = unicode::utf8len(chars[i]);
        std::string curr_str;
        std::vector<std::string>::iterator it_end = it + curr_length;
        while (it != it_end)
        {
          curr_str += *it;
          it++;
        }
        word_tc.push_back(curr_str);
      }
      chars.swap(word_tc);
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
