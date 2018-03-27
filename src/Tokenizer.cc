#include "onmt/Tokenizer.h"

#include <algorithm>
#include <mutex>

#include "onmt/Alphabet.h"
#include "onmt/CaseModifier.h"
#include "onmt/unicode/Unicode.h"

namespace onmt
{

  const std::string Tokenizer::joiner_marker("￭");
  const std::string Tokenizer::spacer_marker("▁");  // same as systran::tokenizer::sp_spacer
  const std::map<std::string, std::string> substitutes = {
                                                      { "￭", "■" },
                                                      { "￨", "│" },
                                                      { "％", "%" },
                                                      { "＃", "#" },
                                                      { "：", ":" }};
  const std::string Tokenizer::ph_marker_open = "｟";
  const std::string Tokenizer::ph_marker_close = "｠";
  const std::string protected_character = "％";

  const std::unordered_map<std::string, onmt::Tokenizer::Mode> Tokenizer::mapMode = {
    { "aggressive", onmt::Tokenizer::Mode::Aggressive },
    { "conservative", onmt::Tokenizer::Mode::Conservative },
    { "space", onmt::Tokenizer::Mode::Space }
  };

  static std::unordered_map<std::string, BPE*> bpe_cache;
  static std::mutex bpe_cache_mutex;

  static BPE* load_bpe(const std::string& bpe_model_path)
  {
    std::lock_guard<std::mutex> lock(bpe_cache_mutex);

    auto it = bpe_cache.find(bpe_model_path);
    if (it != bpe_cache.end())
      return it->second;

    BPE* bpe = new BPE(bpe_model_path);
    bpe_cache[bpe_model_path] = bpe;
    return bpe;
  }

  Tokenizer::Tokenizer(Mode mode,
                       int flags,
                       const std::string& bpe_model_path,
                       const std::string& joiner)
    : _mode(mode)
    , _case_feature(flags & Flags::CaseFeature)
    , _joiner_annotate(flags & Flags::JoinerAnnotate)
    , _joiner_new(flags & Flags::JoinerNew)
    , _with_separators(flags & Flags::WithSeparators)
    , _segment_case(flags & Flags::SegmentCase)
    , _segment_numbers(flags & Flags::SegmentNumbers)
    , _segment_alphabet_change(flags & Flags::SegmentAlphabetChange)
    , _cache_bpe_model(flags & Flags::CacheBPEModel)
    , _no_substitution(flags & Flags::NoSubstitution)
    , _spacer_annotate(flags & Flags::SpacerAnnotate)
    , _bpe(nullptr)
    , _joiner(joiner)
  {
    if (_spacer_annotate)
    {
      _joiner_annotate = true;
      _joiner_new = false;
    }

    set_bpe_model(bpe_model_path, _cache_bpe_model);
  }

  Tokenizer::~Tokenizer()
  {
    if (!_cache_bpe_model)
      delete _bpe;
  }

  std::string Tokenizer::detokenize(const std::vector<std::string>& words,
                                    const std::vector<std::vector<std::string> >& features) const
  {
    std::string line;

    for (size_t i = 0; i < words.size(); ++i)
    {
      if (i > 0 && !has_right_join(words[i - 1]) && !has_left_join(words[i]) &&
        !_spacer_annotate)
        line += " ";

      std::string word = words[i];

      if (has_right_join(word))
        word.erase(word.length() - _joiner.length(), _joiner.length());
      if (has_left_join(word))
        word.erase(0, _joiner.length());

      if (has_right_marker(word, Tokenizer::spacer_marker))
      {
        word.erase(word.length() - Tokenizer::spacer_marker.length(), Tokenizer::spacer_marker.length());
        line += " ";
      }
      else if (has_left_marker(word, Tokenizer::spacer_marker))
      {
        word.erase(0, Tokenizer::spacer_marker.length());
        line += " ";
      }

      if (_case_feature)
      {
        if (features.empty())
          throw std::runtime_error("Missing case feature");
        word = CaseModifier::apply_case(word, features[0][i][0]);
      }

      line += word;
    }

    return line;
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features) const {
    std::unordered_map<std::string,size_t> alphabets;
    return tokenize(text, words, features, alphabets);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features,
                           std::unordered_map<std::string,size_t> &alphabets) const
  {
    if (_mode == Mode::Space) {
      if (text.empty())
        return;

      std::vector<std::string> chunks = unicode::split_utf8(text, " ");
      for (const auto& chunk: chunks)
      {
        if (chunk.empty())
          continue;

        std::vector<std::string> fields = unicode::split_utf8(chunk, ITokenizer::feature_marker);

        words.push_back(fields[0]);

        for (size_t i = 1; i < fields.size(); ++i)
        {
          if (features.size() < i)
            features.emplace_back(1, fields[i]);
          else
            features[i - 1].push_back(fields[i]);
        }
      }
    }
    else {
      std::vector<std::string> chars;
      std::vector<unicode::code_point_t> code_points;

      unicode::explode_utf8(text, chars, code_points);

      std::string token;

      bool letter = false;
      bool uppercase = false;
      bool uppercase_sequence = false;
      bool number = false;
      bool other = false;
      bool space = true;
      bool placeholder = false;
      std::string prev_alphabet;

      unicode::_type_letter type_letter;

      for (size_t i = 0; i < chars.size(); ++i)
      {
        std::string c = chars[i];
        unicode::code_point_t v = code_points[i];
        unicode::code_point_t next_v = i + 1 < code_points.size() ? code_points[i + 1] : 0;
        bool isSeparator = unicode::is_separator(v);

        if (placeholder) {
            if (c == Tokenizer::ph_marker_close) {
              token = token + c;
              letter = true;
              prev_alphabet = "placeholder";
              placeholder = false;
              space = false;
            } else {
              if (isSeparator) {
                char buffer[10];
                sprintf(buffer, "%04x", v);
                c = protected_character + buffer;
              }
              token += c;
            }
          }
          else if (c == Tokenizer::ph_marker_open) {
            std::string initc;
            if (!space) {
              if (_joiner_annotate && !_joiner_new) {
                if ((letter && prev_alphabet != "placeholder") || number)
                  initc = _joiner;
                else
                  token += _joiner;
              }
              words.push_back(token);
              token = initc;
              if (_joiner_annotate && _joiner_new)
                words.push_back(_joiner);
            } else if (other) {
              if (_joiner_annotate && token.length() == 0) {
                if (_joiner_new) words.push_back(_joiner);
                else words[words.size()-1] += _joiner;
              }
            }
            token += c;
            placeholder = true;
        }
        else if (isSeparator)
        {
          if (!space)
          {
            words.push_back(token);
            token.clear();
          }

          if (v == 0x200D) // Zero-Width joiner.
          {
            if (_joiner_annotate)
            {
              if (_joiner_new && !words.empty())
                words.push_back(_joiner);
              else
              {
                if (other || (number && unicode::is_letter(next_v, type_letter)))
                  words.back() += _joiner;
                else
                  token = _joiner;
              }
            }
          }
          else if (_with_separators)
          {
            token += c;
            if (!unicode::is_separator(next_v))
            {
              words.push_back(token);
              token.clear();
            }
          }

          letter = false;
          uppercase = false;
          uppercase_sequence = false;
          number = false;
          other = false;
          space = true;
        }
        else
        {
          bool cur_letter = false;
          bool cur_number = false;
          // skip special characters and BOM
          if (v > 32 && v != 0xFEFF)
          {
            if (!_no_substitution && substitutes.find(c) != substitutes.end())
              c = substitutes.at(c);
            cur_letter = unicode::is_letter(v, type_letter);
            cur_number = unicode::is_number(v);

            std::string alphabet = get_alphabet(v);
            if (!alphabet.empty() && cur_letter)
            {
              if (alphabets.find(alphabet) != alphabets.end()) alphabets.at(alphabet)++;
              else alphabets.insert(std::make_pair(alphabet,1));
            }
            else if (cur_number) {
              if (alphabets.find("Numeric") != alphabets.end()) alphabets.at("Numeric")++;
              else alphabets.insert(std::make_pair("Numeric",1));
	    }
	    else {
              if (alphabets.find("Other") != alphabets.end()) alphabets.at("Other")++;
              else alphabets.insert(std::make_pair("Other",1));
            }

            if (unicode::is_mark(v)) {
              // if we have a mark, we keep type of previous character
              cur_letter = letter;
              cur_number = number;
            }

            if (_mode == Mode::Conservative)
            {
              if (cur_number
                  || (c == "-" && letter)
                  || (c == "_")
                  || (letter && (c == "." || c == ",") && (unicode::is_number(next_v) || unicode::is_letter(next_v, type_letter))))
                {
                  cur_letter = true;
                  alphabet = "Number";
                }
            }

            if (cur_letter)
            {
              if ((!letter && !space)
                  || (letter && !unicode::is_mark(v) &&
                      ((prev_alphabet == alphabet && is_alphabet_to_segment(alphabet))
                       || (prev_alphabet != alphabet && _segment_alphabet_change)
                       || (prev_alphabet == "placeholder"
                           || (_segment_case && letter
                               && ((type_letter == unicode::_letter_upper && !uppercase)
                                   || (type_letter == unicode::_letter_lower && uppercase_sequence)))))))
              {
                if (_joiner_annotate && !_joiner_new)
                  token += _joiner;
                words.push_back(token);
                if (_joiner_annotate && _joiner_new)
                  words.push_back(_joiner);
                token.clear();
                uppercase = (type_letter == unicode::_letter_upper);
                uppercase_sequence = false;
              }
              else if (other && _joiner_annotate && token.empty())
              {
                if (_joiner_new)
                  words.push_back(_joiner);
                else
                  words.back() += _joiner;
                uppercase = (type_letter == unicode::_letter_upper);
                uppercase_sequence = false;
              } else {
                uppercase_sequence = (type_letter == unicode::_letter_upper) & uppercase;
                uppercase = (type_letter == unicode::_letter_upper);
              }

              token += c;
              letter = true;
              number = false;
              other = false;
              space = false;
              prev_alphabet = alphabet;
            }
            else if (cur_number)
            {
              if (letter || (number && _segment_numbers) || (!number && !space))
              {
                bool addjoiner = false;
                if (_joiner_annotate) {
                  if (_joiner_new) addjoiner = true;
                  else {
                    if (!letter || prev_alphabet == "placeholder")
                      token += _joiner;
                    else
                      c = _joiner + c;
                  }
                }
                words.push_back(token);
                if (addjoiner) words.push_back(_joiner);
                token.clear();
              }
              else if (other && _joiner_annotate)
              {
                if (_joiner_new)
                  words.push_back(_joiner);
                else
                  words[words.size()-1] += _joiner;
              }

              token += c;
              letter = false;
              uppercase = false;
              uppercase_sequence = false;
              number = true;
              other = false;
              space = false;
            }
            else
            {
              if (!space)
              {
                words.push_back(token);
                if (_joiner_annotate && _joiner_new)
                  words.push_back(_joiner);
                token.clear();
                if (_joiner_annotate && !_joiner_new)
                  token += _joiner;
              }
              else if (other && _joiner_annotate)
              {
                if (_joiner_new)
                  words.push_back(_joiner);
                else
                  token = _joiner;
              }

              token += c;
              words.push_back(token);
              token.clear();
              letter = false;
              uppercase = false;
              uppercase_sequence = false;
              number = false;
              other = true;
              space = true;
            }
          }
        }
      }

      if (!token.empty())
        words.push_back(token);
    }

    if (_bpe)
      words = bpe_segment(words);

    if (_case_feature)
    {
      std::vector<std::string> case_feat;

      for (size_t i = 0; i < words.size(); ++i)
      {
        if (words[i].find(ph_marker_open) == std::string::npos)
        {
          auto data = CaseModifier::extract_case(words[i]);
          words[i] = data.first;
          case_feat.emplace_back(1, data.second);
        } else
        {
          case_feat.emplace_back(1, 'N');
        }
      }

      features.push_back(case_feat);
    }

    if (_spacer_annotate)
    {
      size_t words_count = words.size();
      words.insert(words.begin(), _joiner);

      for (size_t i = 0; i < words_count; ++i)
      {
        bool has_joiner = false;

        if (has_right_join(words[i]))
        {
          words[i].erase(words[i].length() - _joiner.length());
          has_joiner = true;
        }

        if (has_left_join(words[i+1]))
        {
          words[i+1].erase(0, _joiner.length());
          has_joiner = true;
        }

        if (has_joiner == false)  
        {
          words[i+1] = Tokenizer::spacer_marker + words[i+1];
        }
      } 
      words.erase(words.begin());
    }
  }

  std::vector<std::string> Tokenizer::bpe_segment(const std::vector<std::string>& tokens) const
  {
    std::vector<std::string> segments;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      std::string token = tokens[i];

      if (token.find(Tokenizer::ph_marker_open) != std::string::npos) {
        segments.push_back(token);
        continue;
      }

      bool left_sep = false;
      bool right_sep = false;

      if (_joiner_annotate && !_joiner_new)
      {
        if (has_left_join(token))
        {
          token.erase(0, _joiner.size());
          left_sep = true;
        }

        if (has_right_join(token))
        {
          token.erase(token.size() - _joiner.size());
          right_sep = true;
        }
      }

      auto encoded = _bpe->encode(token);

      if (_joiner_annotate && !_joiner_new)
      {
        if (left_sep)
          encoded.front().insert(0, _joiner);
        if (right_sep)
          encoded.back().append(_joiner);
      }

      for (size_t j = 0; j < encoded.size(); ++j)
      {
        segments.push_back(encoded[j]);

        if (_joiner_annotate && j + 1 < encoded.size())
        {
          if (_joiner_new)
            segments.push_back(_joiner);
          else
            segments.back().append(_joiner);
        }
      }
    }

    return segments;
  }

  Tokenizer& Tokenizer::set_joiner(const std::string& joiner)
  {
    _joiner = joiner;
    return *this;
  }

  Tokenizer& Tokenizer::set_bpe_model(const std::string& model_path, bool cache_model)
  {
    if (_bpe != nullptr && !_cache_bpe_model)
    {
      delete _bpe;
    }

    if (!model_path.empty())
    {
      if (cache_model)
        _bpe = load_bpe(model_path);
      else
        _bpe = new BPE(model_path);

      _cache_bpe_model = cache_model;
    }

    return *this;
  }

  bool Tokenizer::add_alphabet_to_segment(const std::string& alphabet)
  {
    if (!onmt::alphabet_is_supported(alphabet))
      return false;
    _segment_alphabet.insert(alphabet);
    return true;
  }

  bool Tokenizer::is_alphabet_to_segment(const std::string& alphabet) const
  {
    return _segment_alphabet.count(alphabet) > 0;
  }

  bool Tokenizer::has_left_join(const std::string& word) const
  {
    return has_left_marker(word, _joiner);
  }

  bool Tokenizer::has_right_join(const std::string& word) const
  {
    return has_right_marker(word, _joiner);
  }

  bool Tokenizer::has_left_marker(const std::string& word, const std::string& marker) const
  {
    return (word.length() >= marker.length() && word.substr(0, marker.length()) == marker);
  }

  bool Tokenizer::has_right_marker(const std::string& word, const std::string& marker) const
  {
    return (word.length() >= marker.length()
            && word.substr(word.length() - marker.length(), marker.length()) == marker);
  }

}
