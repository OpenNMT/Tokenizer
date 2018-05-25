#include "onmt/Tokenizer.h"

#include <algorithm>
#include <mutex>

#include "onmt/Alphabet.h"
#include "onmt/CaseModifier.h"
#include "onmt/BPE.h"
#ifdef WITH_SP
#  include "onmt/SentencePiece.h"
#endif
#include "onmt/unicode/Unicode.h"

namespace onmt
{

  const std::string Tokenizer::joiner_marker("￭");
  const std::string Tokenizer::spacer_marker("▁");
  const std::map<std::string, std::string> substitutes = {
                                                      { "▁", "_" },
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
    { "space", onmt::Tokenizer::Mode::Space },
    { "char", onmt::Tokenizer::Mode::Char },
    { "none", onmt::Tokenizer::Mode::None }
  };

  static std::unordered_map<std::string, const SubwordEncoder*> cache;
  static std::mutex cache_mutex;

  template <typename T>
  static const T* load_subword_encoder(const std::string& model_path)
  {
    std::lock_guard<std::mutex> lock(cache_mutex);

    auto it = cache.find(model_path);
    if (it != cache.end())
      return dynamic_cast<const T*>(it->second);

    T* encoder = new T(model_path);
    cache[model_path] = encoder;
    return encoder;
  }

  Tokenizer::Tokenizer(Mode mode,
                       int flags,
                       const std::string& model_path,
                       const std::string& joiner)
    : _mode(mode)
    , _case_feature(flags & Flags::CaseFeature)
    , _joiner_annotate(flags & Flags::JoinerAnnotate)
    , _joiner_new(flags & Flags::JoinerNew)
    , _with_separators(flags & Flags::WithSeparators)
    , _segment_case(flags & Flags::SegmentCase)
    , _segment_numbers(flags & Flags::SegmentNumbers)
    , _segment_alphabet_change(flags & Flags::SegmentAlphabetChange)
    , _cache_model((flags & Flags::CacheBPEModel) | (flags & Flags::CacheModel))
    , _no_substitution(flags & Flags::NoSubstitution)
    , _spacer_annotate(flags & Flags::SpacerAnnotate)
    , _spacer_new(flags & Flags::SpacerNew)
    , _preserve_placeholders(flags & Flags::PreservePlaceholders)
    , _subword_encoder(nullptr)
    , _joiner(joiner)
  {
    if (flags & Flags::SentencePieceModel)
#ifdef WITH_SP
      set_sp_model(model_path, _cache_model);
    else
#else
      throw std::runtime_error("The Tokenizer was not built with SentencePiece support");
#endif
    {
      set_bpe_model(model_path, _cache_model);
    }
  }

  Tokenizer::~Tokenizer()
  {
    if (!_cache_model)
      delete _subword_encoder;
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
        if (i > 0)
          line += " ";
      }
      else if (has_left_marker(word, Tokenizer::spacer_marker))
      {
        word.erase(0, Tokenizer::spacer_marker.length());
        if (i > 0)
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
    std::unordered_map<std::string, size_t> alphabets;
    return tokenize(text, words, features, alphabets);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features,
                           std::unordered_map<std::string, size_t>& alphabets) const
  {
    std::vector<AnnotatedToken> annotated_tokens;

    if (_mode == Mode::None) {
      annotated_tokens.emplace_back(text);
    } else if (_mode == Mode::Space) {
      if (text.empty())
        return;

      std::vector<std::string> chunks = unicode::split_utf8(text, " ");
      for (const auto& chunk: chunks)
      {
        if (chunk.empty())
          continue;

        std::vector<std::string> fields = unicode::split_utf8(chunk, ITokenizer::feature_marker);

        annotated_tokens.emplace_back(fields[0]);

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

      AnnotatedToken token;

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
              token.append(c);
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
              token.append(c);
            }
          }
          else if (c == Tokenizer::ph_marker_open) {
            if (!space) {
              AnnotatedToken next_token;
              if ((letter && prev_alphabet != "placeholder") || number)
                next_token.join_left();
              else
                token.join_right();
              annotated_tokens.push_back(token);
              token = next_token;
            } else if (other && token.str().empty()) {
              annotated_tokens.back().join_right();
            }
            token.append(c);
            placeholder = true;
        }
        else if (isSeparator)
        {
          if (!space)
          {
            annotated_tokens.push_back(token);
            token.clear();
          }

          if (v == 0x200D) // Zero-Width joiner.
          {
            if (other || (number && unicode::is_letter(next_v, type_letter)))
              annotated_tokens.back().join_right();
            else
            {
              token.clear();
              token.join_left();
            }
          }
          else if (_with_separators)
          {
            token.append(c);
            if (!unicode::is_separator(next_v))
            {
              annotated_tokens.push_back(token);
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
              alphabets[alphabet]++;
            else
              alphabets[cur_number ? "Numeric" : "Other"]++;

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

            if (cur_letter && _mode != Mode::Char)
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
                token.join_right();
                annotated_tokens.push_back(token);
                token.clear();
                uppercase = (type_letter == unicode::_letter_upper);
                uppercase_sequence = false;
              }
              else if (other && token.str().empty())
              {
                annotated_tokens.back().join_right();
                uppercase = (type_letter == unicode::_letter_upper);
                uppercase_sequence = false;
              } else {
                uppercase_sequence = (type_letter == unicode::_letter_upper) & uppercase;
                uppercase = (type_letter == unicode::_letter_upper);
              }

              token.append(c);
              letter = true;
              number = false;
              other = false;
              space = false;
              prev_alphabet = alphabet;
            }
            else if (cur_number && _mode != Mode::Char)
            {
              if (letter || (number && _segment_numbers) || (!number && !space))
              {
                AnnotatedToken next_token;
                if (!letter || prev_alphabet == "placeholder")
                  token.join_right();
                else
                  next_token.join_left();
                annotated_tokens.push_back(token);
                token = next_token;
              }
              else if (other)
              {
                annotated_tokens.back().join_right();
              }

              token.append(c);
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
                annotated_tokens.push_back(token);
                token.clear();
                token.join_left();
              }
              else if (other)
              {
                token.clear();
                token.join_left();
              }

              token.append(c);
              annotated_tokens.push_back(token);
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

      if (!token.str().empty())
        annotated_tokens.push_back(token);
    }

    if (_subword_encoder)
      annotated_tokens = encode_subword(annotated_tokens);

    if (_case_feature)
    {
      std::vector<std::string> case_feat;

      for (size_t i = 0; i < annotated_tokens.size(); ++i)
      {
        if (!is_placeholder(annotated_tokens[i].str()))
        {
          auto data = CaseModifier::extract_case(annotated_tokens[i].str());
          annotated_tokens[i].set(data.first);
          case_feat.emplace_back(1, data.second);
        }
        else
        {
          case_feat.emplace_back(1, CaseModifier::type_to_char(CaseModifier::Type::None));
        }
      }

      features.push_back(case_feat);
    }

    finalize_tokens(annotated_tokens, words);
  }

  void Tokenizer::finalize_tokens(std::vector<AnnotatedToken>& annotated_tokens,
                                  std::vector<std::string>& tokens) const
  {
    for (size_t i = 0; i < annotated_tokens.size(); ++i)
    {
      const auto& token = annotated_tokens[i];

      if (_joiner_annotate)
      {
        if (token.is_joined_left() && i > 0)
        {
          if (_joiner_new)
          {
            tokens.push_back(_joiner);
            if (!token.str().empty())
              tokens.push_back(token.str());
          }
          else if (_preserve_placeholders && is_placeholder(token.str()))
          {
            tokens.back() += _joiner;
            tokens.push_back(token.str());
          }
          else
            tokens.push_back(_joiner + token.str());
        }
        else if (!token.str().empty())
          tokens.push_back(token.str());
        if (token.is_joined_right() && i + 1 < annotated_tokens.size())
        {
          if (_joiner_new)
            tokens.push_back(_joiner);
          else if (_preserve_placeholders && is_placeholder(token.str()))
          {
            auto& next_token = annotated_tokens[i + 1];
            if (!is_placeholder(next_token.str()))
              next_token.join_left();
            else
              tokens.push_back(_joiner);
          }
          else
            tokens.back() += _joiner;
        }
      }
      else if (_spacer_annotate)
      {
        bool joined_left = (token.is_joined_left()
                            || (i > 0 && annotated_tokens[i - 1].is_joined_right()));
        if (joined_left || (i == 0 && !token.is_spacer()))
        {
          if (!token.str().empty())
            tokens.push_back(token.str());
        }
        else if (_preserve_placeholders && is_placeholder(token.str()))
        {
          tokens.push_back(spacer_marker);
          tokens.push_back(token.str());
        }
        else
        {
          if (_spacer_new)
          {
            tokens.push_back(spacer_marker);
            tokens.push_back(token.str());
          }
          else
            tokens.push_back(spacer_marker + token.str());
        }
      }
      else if (!token.str().empty())
      {
        tokens.push_back(token.str());
      }
    }
  }

  std::vector<AnnotatedToken> Tokenizer::encode_subword(
      const std::vector<AnnotatedToken>& tokens) const
  {
    std::vector<AnnotatedToken> segments;

    for (const auto& token : tokens)
    {
      if (is_placeholder(token.str())) {
        segments.push_back(token);
        continue;
      }

      std::vector<AnnotatedToken> sub_segments = _subword_encoder->encode_and_annotate(token);
      segments.insert(segments.end(), sub_segments.begin(), sub_segments.end());
    }

    return segments;
  }

  Tokenizer& Tokenizer::set_joiner(const std::string& joiner)
  {
    _joiner = joiner;
    return *this;
  }

  template <typename T>
  Tokenizer& Tokenizer::set_subword_encoder_model(const std::string& model_path, bool cache_model)
  {
    if (_subword_encoder != nullptr && !_cache_model)
    {
      delete _subword_encoder;
    }

    if (!model_path.empty())
    {
      if (cache_model)
        _subword_encoder = load_subword_encoder<T>(model_path);
      else
        _subword_encoder = new T(model_path);

      _cache_model = cache_model;
    }

    return *this;
  }

  Tokenizer& Tokenizer::set_bpe_model(const std::string& model_path, bool cache_model)
  {
    return this->set_subword_encoder_model<BPE>(model_path, cache_model);
  }

#ifdef WITH_SP
  Tokenizer& Tokenizer::set_sp_model(const std::string& model_path, bool cache_model)
  {
    if (_mode == Mode::None && !_joiner_annotate && !_spacer_annotate)
      _spacer_annotate = true;
    return this->set_subword_encoder_model<SentencePiece>(model_path, cache_model);
  }
#endif

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

  bool Tokenizer::is_placeholder(const std::string& str)
  {
    return str.find(ph_marker_open) == 0;
  }

}
