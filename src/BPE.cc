#include "onmt/BPE.h"
#include <fstream>
#include <limits>

#include "onmt/unicode/Unicode.h"
#include "onmt/CaseModifier.h"

namespace onmt
{

  BPE::BPE(const std::string& model_path)
    : _end_of_word("</w>")
    , _begin_of_word("<w>")
    , _prefix(false)
    , _suffix(true)
    , _case_insensitive(false)
    , _version(0, 0)
    , _joiner("")
  {
    std::ifstream in(model_path.c_str());

    if (!in.is_open())
      throw std::invalid_argument("Unable to open BPE model `" + model_path + "'");

    std::string line;

    int i = 0;

    std::getline(in, line);

    if (line.compare(0, 9, "#version:") == 0)  // Model from learn_bpe.py
    {
      int major_version = line[line.size() - 3] - '0';
      int minor_version = line[line.size() - 1] - '0';
      _version = std::make_pair(major_version, minor_version);
    }
    else  // Model possibly from learn_bpe.lua
    {
      std::vector<std::string> options;
      std::string option;

      size_t sep = line.find(';');
      size_t bidx = 0;
      while (sep != std::string::npos && sep + 1 < line.size())
      {
        options.push_back(line.substr(bidx, sep - bidx));
        bidx = sep + 1;
        sep = line.find(';', bidx);
      }
      options.push_back(line.substr(bidx));

      if (options.size() == 6 && options[0] == "v3")
      {
        _prefix = options[1] == "true";
        _suffix = options[2] == "true";
        _case_insensitive = options[3] == "true";
        _begin_of_word = options[4];
        _end_of_word = options[5];
      }
      else  // Model from learn_bpe.py v0.1
        in.seekg(0);
    }

    while (std::getline(in, line))
    {
      size_t sep = line.find(' ');
      if (sep != std::string::npos && sep + 1 < line.size())
      {
        std::string first_token = line.substr(0, sep);
        std::string second_token = line.substr(sep + 1);
        std::string pair = first_token + second_token;
        if (_codes.count(pair) == 0)
          _codes.emplace(pair, i++);

        _codes_reverse.emplace(pair, std::pair<std::string, std::string>(first_token, second_token));
      }
    }
  }

  std::vector<std::string> BPE::encode(const std::string& str) const
  {
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;

    if (_case_insensitive)
      unicode::explode_utf8(CaseModifier::extract_case(str).first, chars, code_points);
    else
      unicode::explode_utf8(str, chars, code_points);

    if (chars.size() == 1)
    {
      chars[0] = str;
      return chars;
    }

    if (_version.first != 0 || _version.second != 0)
    {
      if (_version.first == 0 && _version.second == 1)
        chars.push_back(_end_of_word);
      else if (_version.first == 0 && _version.second == 2)
        chars.back() += _end_of_word;
      else
        throw std::runtime_error("unsupported BPE version");
    }
    else
    {
      if (_prefix)
        chars.insert(chars.begin(), _begin_of_word);
      if (_suffix)
        chars.push_back(_end_of_word);
    }

    apply_merges(chars);

    if (_prefix)
    {
      if (chars.front() == _begin_of_word)
        chars.erase(chars.begin());
      else if (chars.front().compare(0, _begin_of_word.size(), _begin_of_word) == 0)
        chars.front().erase(0, _begin_of_word.size());
    }

    if (chars.back() == _end_of_word)
      chars.pop_back();
    else if (chars.back().size() > _end_of_word.size()
             && chars.back().compare(chars.back().size() - _end_of_word.size(),
                                     std::string::npos,
                                     _end_of_word) == 0)
      chars.back().erase(chars.back().size() - _end_of_word.size(), _end_of_word.size());

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

    if (!_bpe_vocab.empty())
    {
      std::vector<std::string> out;
      check_vocab_and_split(chars, out);
      chars.swap(out);
    }

    return chars;
  }

  void BPE::apply_merges(std::vector<std::string>& chars) const
  {
    std::vector<std::string> new_chars;
    new_chars.reserve(chars.size());

    while (true)
    {
      int min_index = get_min_pair_index(chars);

      if (min_index < 0)
        break;

      const std::string& gram1 = chars[min_index];
      const std::string& gram2 = chars[min_index + 1];

      bool merge = false;
      new_chars.clear();

      for (size_t i = 0; i < chars.size(); ++i)
      {
        if (merge)
        {
          if (chars[i] == gram2)
          {
            new_chars.push_back(gram1 + gram2);
            merge = false;
          }
          else if (chars[i] == gram1)
          {
            new_chars.push_back(gram1);
          }
          else
          {
            new_chars.push_back(gram1);
            new_chars.push_back(chars[i]);
            merge = false;
          }
        }
        else
        {
          if (chars[i] == gram1)
            merge = true;
          else
            new_chars.push_back(chars[i]);
        }
      }

      chars.swap(new_chars);
      if (chars.size() == 1)
        break;
    }
  }

  int BPE::get_min_pair_index(const std::vector<std::string>& chars) const
  {
    int min_index = -1;
    int min_score = std::numeric_limits<int>::max();

    for (int i = 0; i + 1 < static_cast<int>(chars.size()); ++i)
    {
      auto it = _codes.find(chars[i] + chars[i + 1]);

      if (it != _codes.end())
      {
        int score = it->second;
        if (score < min_score)
        {
          min_score = score;
          min_index = i;
        }
      }
    }

    return min_index;
  }

  void BPE::init_bpe_vocab(const std::string& vocab_path, int bpe_vocab_threshold)
  {
    if (!_bpe_vocab.empty())
      return;

    std::ifstream in(vocab_path.c_str());

    if (!in.is_open())
      throw std::invalid_argument("Unable to open BPE vocab file `" + vocab_path + "'");

    std::string line;
    while (std::getline(in, line))
    {
      size_t sep = line.find(' ');
      if (sep != std::string::npos)
      {
        if(std::stoi(line.substr(sep + 1)) >= bpe_vocab_threshold)
          _bpe_vocab.emplace(line.substr(0, sep));
      }
    }
  }

  void BPE::check_vocab_and_split(const std::vector<std::string> & orig, std::vector<std::string> & out) const
  {
    // Check for each segment in word if it is in-vocabulary,
    // and segment OOV segments into smaller units by reversing the BPE merge operations

    for (auto it = orig.begin(); it != orig.end(); ++it)
    {
      const std::string& segment = *it;

      // if it is not the last, add joiner
      if (_bpe_vocab.count(std::next(it) == orig.end() ? segment : segment+_joiner) > 0)
      {
        out.push_back(segment);
      }
      else
      {
        recursive_split(segment, out, std::next(it) == orig.end());
      }
    }

  }

  void BPE::recursive_split(const std::string & segment, std::vector<std::string> & out, bool finalflag) const
  {
    // Recursively split segment into smaller units (by reversing BPE merges)
    // until all units are either in - vocabulary, or cannot be split futher.

    auto got = _codes_reverse.find(finalflag == true ? segment+_end_of_word: segment);
    if (got != _codes_reverse.end())
    {
      std::string left = got->second.first;
      std::string right = got->second.second;

      if (finalflag)
        right = right.substr(0, right.size() - 4);

      recursive_split_left(left, out);
      recursive_split_right(right, out, finalflag);
    }
    else
    {
      out.push_back(segment);
    }

  }

  void BPE::recursive_split_left(const std::string & segment, std::vector<std::string> & out) const
  {
    if (_bpe_vocab.count(segment + _joiner) > 0)
    {
      out.push_back(segment);
    }
    else
    {
      recursive_split(segment, out, false);
    }

  }

  void BPE::recursive_split_right(const std::string & segment, std::vector<std::string> & out, bool finalflag) const
  {
    if((finalflag && _bpe_vocab.count(segment) > 0) || (!finalflag && _bpe_vocab.count(segment + _joiner) > 0))
    {
      out.push_back(segment);
    }
    else
    {
      recursive_split(segment, out, finalflag);
    }

  }
}
