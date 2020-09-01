#include "onmt/BPE.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <random>

#include "onmt/unicode/Unicode.h"
#include "Casing.h"

namespace onmt
{

  static std::mt19937& get_random_generator()
  {
    static thread_local std::mt19937 generator(
      std::chrono::system_clock::now().time_since_epoch().count());
    return generator;
  }

  static inline float check_dropout(const float dropout)
  {
    if (dropout < 0 || dropout > 1)
      throw std::invalid_argument("bpe_dropout should be between 0 and 1");
    return dropout;
  }


  BPE::BPE(const std::string& model_path, const float dropout)
    : _end_of_word("</w>")
    , _begin_of_word("<w>")
    , _prefix(false)
    , _suffix(true)
    , _case_insensitive(false)
    , _version(0, 0)
    , _joiner("")
    , _dropout(check_dropout(dropout))
  {
    load_model(model_path);
  }

  BPE::BPE(const std::string& model_path, const std::string& joiner, const float dropout)
    : _end_of_word("</w>")
    , _begin_of_word("<w>")
    , _prefix(false)
    , _suffix(true)
    , _case_insensitive(false)
    , _version(0, 0)
    , _joiner(joiner)
    , _dropout(check_dropout(dropout))
  {
    load_model(model_path);
  }

  void BPE::load_model(const std::string& model_path)
  {
    std::ifstream in(model_path.c_str());

    if (!in)
      throw std::invalid_argument("Unable to open BPE model " + model_path);

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

    bool header = true;
    while (std::getline(in, line))
    {
      /* line starting with '#' at the beginning of the file is a header */
      if (header && line.length() && line[0] == '#')
        continue;
      header = false;
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

    if (_case_insensitive)
      unicode::explode_utf8_with_marks(lowercase_token(str).first, chars);
    else
      unicode::explode_utf8_with_marks(str, chars);

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
      // Return true case pieces.
      std::vector<std::string> words_tc;
      words_tc.reserve(chars.size());

      std::vector<std::string> chars_tc;
      std::vector<unicode::code_point_t> code_points_tc;
      unicode::explode_utf8(str, chars_tc, code_points_tc);

      for (size_t word_index = 0, char_index = 0; word_index < chars.size(); ++word_index)
      {
        // We accumulate true case characters from the original input (str) until the length
        // of their lower case version (length_lc) matches the length of lowercase word that
        // was merged by BPE (word_lc).
        const std::string& word_lc = chars[word_index];
        std::string word_tc;
        for (size_t length_lc = 0;
             char_index < code_points_tc.size() && length_lc < word_lc.size();
             ++char_index)
        {
          const unicode::code_point_t cp_lc = unicode::get_lower(code_points_tc[char_index]);
          const std::string char_lc = unicode::cp_to_utf8(cp_lc);
          length_lc += char_lc.size();
          word_tc += chars_tc[char_index];
        }
        words_tc.emplace_back(std::move(word_tc));
      }

      chars = std::move(words_tc);
    }

    if (!_bpe_vocab.empty())
    {
      std::vector<std::string> out;
      check_vocab_and_split(chars, out);
      chars.swap(out);
    }

    return chars;
  }

  int BPE::get_score(const std::string& gram1, const std::string& gram2) const
  {
    auto it = _codes.find(gram1 + gram2);
    if (it != _codes.end())
      return it->second;
    else
      return std::numeric_limits<int>::max();
  }

  void BPE::apply_merges(std::vector<std::string>& chars) const
  {
    // Compute score for all pairs.
    std::vector<int> scores;
    scores.reserve(chars.size() - 1);
    for (size_t i = 0; i + 1 < chars.size(); ++i)
      scores.push_back(get_score(chars[i], chars[i + 1]));

    while (true)
    {
      // Get best score.
      int best_score = std::numeric_limits<int>::max();
      size_t index = 0;

      for (size_t i = 0; i < scores.size(); ++i)
      {
        if (_dropout != 0)
        {
          std::uniform_real_distribution<float> dist;
          const float sample = dist(get_random_generator());
          if (sample < _dropout)
            continue;
        }

        const int score = scores[i];
        if (score < best_score)
        {
          best_score = score;
          index = i;
        }
      }

      if (best_score == std::numeric_limits<int>::max())
        break;

      // Merge pair.
      chars[index] += chars[index + 1];
      chars.erase(chars.begin() + index + 1);
      if (chars.size() == 1)
        break;

      // Update score of pairs (index-1,index) and (index,index+1).
      if (index > 0)
        scores[index - 1] = get_score(chars[index - 1], chars[index]);
      if (index + 1 < chars.size())
        scores[index] = get_score(chars[index], chars[index + 1]);
      scores.erase(scores.begin() + std::min(index + 1, chars.size() - 1));
    }
  }

  void BPE::init_bpe_vocab(const std::string& vocab_path, int bpe_vocab_threshold)
  {
    if (!_bpe_vocab.empty())
      return;

    load_vocabulary(vocab_path, bpe_vocab_threshold);
  }

  void BPE::set_vocabulary(const std::vector<std::string>& vocabulary)
  {
    _bpe_vocab.insert(vocabulary.begin(), vocabulary.end());
  }

  void BPE::reset_vocabulary()
  {
    _bpe_vocab.clear();
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
