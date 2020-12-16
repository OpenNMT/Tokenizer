#include "onmt/BPE.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <random>

#include "onmt/Tokenizer.h"
#include "onmt/unicode/Unicode.h"
#include "Casing.h"
#include "Utils.h"

namespace onmt
{

  static std::mt19937& get_random_generator()
  {
    static thread_local std::mt19937 generator(get_random_generator_seed());
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
    , _dropout(check_dropout(dropout))
  {
    load_model(model_path);

    // For backward compatibility, assume the tokenization uses joiner annotation.
    _tokenization_options.joiner_annotate = true;
    _tokenization_options.joiner = Tokenizer::joiner_marker;
  }

  BPE::BPE(const std::string& model_path, const std::string& joiner, const float dropout)
    : _end_of_word("</w>")
    , _begin_of_word("<w>")
    , _prefix(false)
    , _suffix(true)
    , _case_insensitive(false)
    , _version(0, 0)
    , _dropout(check_dropout(dropout))
  {
    load_model(model_path);

    // For backward compatibility, assume the tokenization uses joiner annotation.
    _tokenization_options.joiner_annotate = true;
    _tokenization_options.joiner = joiner;
  }

  void BPE::load_model(const std::string& model_path)
  {
    std::ifstream in(model_path.c_str());

    if (!in)
      throw std::invalid_argument("Unable to open BPE model " + model_path);

    std::string line;

    int i = 0;

    std::getline(in, line);

    if (starts_with(line, "#version:"))  // Model from learn_bpe.py
    {
      int major_version = line[line.size() - 3] - '0';
      int minor_version = line[line.size() - 1] - '0';
      _version = std::make_pair(major_version, minor_version);
      if (_version != std::make_pair(0, 1)
          && _version != std::make_pair(0, 2))
        throw std::runtime_error("unsupported BPE version");
    }
    else  // Model possibly from learn_bpe.lua
    {
      std::vector<std::string> options;
      options.reserve(6);

      size_t sep = line.find(';');
      size_t bidx = 0;
      while (sep != std::string::npos && sep + 1 < line.size())
      {
        options.emplace_back(line.substr(bidx, sep - bidx));
        bidx = sep + 1;
        sep = line.find(';', bidx);
      }
      options.emplace_back(line.substr(bidx));

      if (options.size() == 6 && options[0] == "v3")
      {
        _prefix = options[1] == "true";
        _suffix = options[2] == "true";
        _case_insensitive = options[3] == "true";
        _begin_of_word = std::move(options[4]);
        _end_of_word = std::move(options[5]);
      }
      else  // Model from learn_bpe.py v0.1
        in.seekg(0);
    }

    bool header = true;
    while (std::getline(in, line))
    {
      /* line starting with '#' at the beginning of the file is a header */
      if (header && !line.empty() && line[0] == '#')
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

        _codes_reverse.emplace(std::move(pair),
                               std::make_pair(std::move(first_token), std::move(second_token)));
      }
    }
  }

  std::vector<std::string> BPE::get_initial_pieces(const std::vector<unicode::CharInfo>& chars,
                                                   const bool lowercase)
  {
    // Merge combining marks and possibly lowercase characters.

    std::vector<std::string> pieces;
    pieces.reserve(chars.size());

    for (const auto& c : chars)
    {
      if (c.char_type == unicode::CharType::Mark)
      {
        if (pieces.empty())
          pieces.emplace_back(c.data, c.length);
        else
          pieces.back().append(c.data, c.length);
      }
      else if (lowercase && c.case_type == unicode::CaseType::Upper)
        pieces.emplace_back(unicode::cp_to_utf8(unicode::get_lower(c.value)));
      else
        pieces.emplace_back(c.data, c.length);
    }

    return pieces;
  }

  std::vector<std::string> BPE::encode(const std::string& str) const
  {
    const auto chars_info = unicode::get_characters_info(str);
    std::vector<std::string> chars = get_initial_pieces(chars_info, _case_insensitive);

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
    }
    else
    {
      if (_prefix)
        chars.insert(chars.begin(), _begin_of_word);
      if (_suffix)
        chars.push_back(_end_of_word);
    }

    apply_merges(chars);

    if (_prefix && starts_with(chars.front(), _begin_of_word))
    {
      if (chars.front().size() == _begin_of_word.size())
        chars.erase(chars.begin());
      else
        chars.front().erase(0, _begin_of_word.size());
    }

    if (_suffix && ends_with(chars.back(), _end_of_word))
    {
      if (chars.back().size() == _end_of_word.size())
        chars.pop_back();
      else
        chars.back().erase(chars.back().size() - _end_of_word.size());
    }

    if (_case_insensitive)
    {
      // Return true case pieces.
      std::vector<std::string> words_tc;
      words_tc.reserve(chars.size());

      for (size_t word_index = 0, char_index = 0; word_index < chars.size(); ++word_index)
      {
        // We accumulate true case characters from the original input (str) until the length
        // of their lower case version (length_lc) matches the length of lowercase word that
        // was merged by BPE (word_lc).
        const std::string& word_lc = chars[word_index];
        std::string word_tc;
        for (size_t length_lc = 0;
             char_index < chars_info.size() && length_lc < word_lc.size();
             ++char_index)
        {
          const auto& char_tc = chars_info[char_index];
          if (char_tc.case_type == unicode::CaseType::Upper)
            length_lc += unicode::cp_to_utf8(unicode::get_lower(char_tc.value)).size();
          else
            length_lc += char_tc.length;
          word_tc.append(char_tc.data, char_tc.length);
        }
        words_tc.emplace_back(std::move(word_tc));
      }

      chars = std::move(words_tc);
    }

    return chars;
  }

  std::vector<Token> BPE::encode_and_annotate(const Token& token) const
  {
    std::vector<std::string> encoded = encode(token.surface);
    std::vector<Token> tokens;
    tokens.reserve(encoded.size());

    for (size_t j = 0; j < encoded.size(); ++j)
    {
      Token subword(std::move(encoded[j]));
      if (j == 0)
      {
        subword.join_left = token.join_left;
        subword.preserve = token.preserve;
      }
      if (j + 1 < encoded.size())
        subword.join_right = true;
      else
      {
        subword.join_right = token.join_right;
        subword.preserve = token.preserve;
      }
      tokens.emplace_back(std::move(subword));
    }

    if (!_bpe_vocab.empty())
      tokens = check_vocab_and_split(std::move(tokens));

    propagate_token_properties(token, tokens);
    return tokens;
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

  void BPE::set_vocabulary(const std::vector<std::string>& vocabulary,
                           const Tokenizer::Options* options)
  {
    _bpe_vocab.clear();
    _bpe_vocab.insert(vocabulary.begin(), vocabulary.end());
    if (options)
      _tokenization_options = *options;
  }

  void BPE::reset_vocabulary()
  {
    _bpe_vocab.clear();
  }

  bool BPE::in_vocabulary(const std::string& token) const
  {
    return _bpe_vocab.find(token) != _bpe_vocab.end();
  }

  bool BPE::in_vocabulary(const onmt::Token& token) const
  {
    std::string surface = token.surface;

    if (!token.preserve)
    {
      if (_tokenization_options.joiner_annotate && !_tokenization_options.joiner_new)
      {
        if (token.join_left)
          surface = _tokenization_options.joiner + surface;
        if (token.join_right)
          surface = surface + _tokenization_options.joiner;
      }
      else if (_tokenization_options.spacer_annotate && !_tokenization_options.spacer_new)
      {
        if (!token.join_left)
          surface = Tokenizer::spacer_marker + surface;
      }
    }

    return in_vocabulary(surface);
  }

  std::vector<Token> BPE::check_vocab_and_split(std::vector<Token> pieces) const
  {
    // Check for each segment in word if it is in-vocabulary,
    // and segment OOV segments into smaller units by reversing the BPE merge operations
    std::vector<Token> pieces_in_vocab;
    pieces_in_vocab.reserve(pieces.size());

    for (size_t i = 0; i < pieces.size(); ++i)
    {
      Token& piece = pieces[i];

      if (in_vocabulary(piece))
        pieces_in_vocab.emplace_back(std::move(piece));
      else
      {
        const bool first = (i == 0);
        const bool last = (i + 1 == pieces.size());
        recursive_split(std::move(piece), pieces_in_vocab, first, last);
      }
    }

    return pieces_in_vocab;
  }

  void BPE::recursive_split(Token piece,
                            std::vector<Token>& pieces_in_vocab,
                            const bool first,
                            const bool last) const
  {
    // Recursively split segment into smaller units (by reversing BPE merges)
    // until all units are either in - vocabulary, or cannot be split further.
    std::string bpe_surface = piece.surface;
    size_t left_offset = 0;
    size_t right_offset = 0;
    if (_prefix && first)
    {
      bpe_surface = _begin_of_word + bpe_surface;
      left_offset = _begin_of_word.size();
    }
    if (_suffix && last)
    {
      bpe_surface = bpe_surface + _end_of_word;
      right_offset = _end_of_word.size();
    }

    auto it = _codes_reverse.find(bpe_surface);
    if (it == _codes_reverse.end())
    {
      pieces_in_vocab.emplace_back(std::move(piece));
      return;
    }

    const auto& pair = it->second;

    {
      Token left_piece(pair.first.substr(left_offset));
      left_piece.join_left = first && piece.join_left;
      left_piece.join_right = true;
      left_piece.preserve = first && piece.preserve;

      if (in_vocabulary(left_piece))
        pieces_in_vocab.emplace_back(std::move(left_piece));
      else
        recursive_split(std::move(left_piece), pieces_in_vocab, first, false);
    }

    {
      Token right_piece(pair.second.substr(0, pair.second.size() - right_offset));
      right_piece.join_left = false;
      right_piece.join_right = !last || piece.join_right;
      right_piece.preserve = last && piece.preserve;

      if (in_vocabulary(right_piece))
        pieces_in_vocab.emplace_back(std::move(right_piece));
      else
        recursive_split(std::move(right_piece), pieces_in_vocab, false, last);
    }
  }

}
