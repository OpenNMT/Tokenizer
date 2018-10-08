#include "onmt/Tokenizer.h"

#include <algorithm>
#include <map>
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

  enum State
  {
    Letter = 1 << 0,
    Number = 1 << 1,
    Space = 1 << 2,
    Other = 1 << 3,
    Placeholder = 1 << 4
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
                       const std::string& joiner,
                       const std::string& bpe_vocab_path,
                       int bpe_vocab_threshold)
    : _mode(mode)
    , _subword_encoder(nullptr)
    , _joiner(joiner)
  {
    read_flags(flags);
    if (flags & Flags::SentencePieceModel)
      set_sp_model(model_path, _cache_model);
    else
    {
      set_bpe_model(model_path, _cache_model);
      if (_subword_encoder != nullptr && !bpe_vocab_path.empty())
      {
        ((BPE *)_subword_encoder)->init_bpe_vocab(bpe_vocab_path, bpe_vocab_threshold);
        ((BPE *)_subword_encoder)->set_joiner(joiner);
      }
    }
  }

  Tokenizer::Tokenizer(Mode mode,
                       const SubwordEncoder* subword_encoder,
                       int flags,
                       const std::string& joiner)
    : _mode(mode)
    , _subword_encoder(subword_encoder)
    , _joiner(joiner)
  {
    read_flags(flags);
    _cache_model = true;
    if (dynamic_cast<const SentencePiece*>(subword_encoder) != nullptr
        && _mode == Mode::None && !_joiner_annotate && !_spacer_annotate)
      _spacer_annotate = true;
  }

  Tokenizer::Tokenizer(const std::string& sp_model_path,
                       int sp_nbest_size,
                       float sp_alpha,
                       Mode mode,
                       int flags,
                       const std::string& joiner)
    : _mode(mode)
    , _subword_encoder(nullptr)
    , _joiner(joiner)
  {
    read_flags(flags);
    set_sp_model(sp_model_path, _cache_model);
    if (sp_nbest_size != 0)
#ifdef SP_HAS_SAMPLE_ENCODE
      ((SentencePiece*)_subword_encoder)->enable_regularization(sp_nbest_size, sp_alpha);
#else
      throw std::runtime_error("This version of SentencePiece does not include the sampling API");
#endif
  }

  void Tokenizer::read_flags(int flags)
  {
    _case_feature = flags & Flags::CaseFeature;
    _case_markup = flags & Flags::CaseMarkup;
    _joiner_annotate = flags & Flags::JoinerAnnotate;
    _joiner_new = flags & Flags::JoinerNew;
    _with_separators = flags & Flags::WithSeparators;
    _segment_case = (flags & Flags::SegmentCase) | (flags & Flags::CaseMarkup);
    _segment_numbers = flags & Flags::SegmentNumbers;
    _segment_alphabet_change = flags & Flags::SegmentAlphabetChange;
    _cache_model = (flags & Flags::CacheBPEModel) | (flags & Flags::CacheModel);
    _no_substitution = flags & Flags::NoSubstitution;
    _spacer_annotate = flags & Flags::SpacerAnnotate;
    _spacer_new = flags & Flags::SpacerNew;
    _preserve_placeholders = flags & Flags::PreservePlaceholders;
    _preserve_segmented_tokens = flags & Flags::PreserveSegmentedTokens;
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
    line.reserve(words.size() * 10);

    CaseModifier::Type case_region = CaseModifier::Type::None;
    ssize_t previous_token = -1;

    for (size_t i = 0; i < words.size(); ++i)
    {
      CaseModifier::Type case_modifier = case_region;

      if (_case_feature)
      {
        if (features.empty())
          throw std::runtime_error("Missing case feature");
        case_modifier = CaseModifier::char_to_type(features[0][i][0]);
      }
      else
      {
        auto case_markup = CaseModifier::get_case_markup(words[i]);
        if (case_markup != CaseModifier::Markup::None)
        {
          if (case_markup == CaseModifier::Markup::RegionEnd)
          {
            case_region = CaseModifier::Type::None;
            continue;
          }
          else
          {
            case_modifier = CaseModifier::get_case_modifier_from_markup(words[i]);
            if (case_markup == CaseModifier::Markup::RegionBegin)
            {
              case_region = case_modifier;
              continue;
            }
            else
              ++i;
          }
        }
      }

      if (previous_token >= 0
          && !has_right_join(words[previous_token]) && !has_left_join(words[i])
          && !_spacer_annotate)
        line += ' ';

      const std::string& word = words[i];
      size_t subpos = 0;
      size_t sublen = word.size();

      if (has_right_join(word))
        sublen -= _joiner.length();
      if (has_left_join(word))
      {
        subpos += _joiner.length();
        sublen -= _joiner.length();
      }

      if (sublen == word.size())
      {
        if (has_right_marker(word, spacer_marker))
        {
          sublen -= spacer_marker.length();
          if (previous_token >= 0)
            line += ' ';
        }
        else if (has_left_marker(word, spacer_marker))
        {
          subpos += spacer_marker.length();
          sublen -= spacer_marker.length();
          if (previous_token >= 0)
            line += ' ';
        }
      }

      if (case_modifier != CaseModifier::Type::None)
        line.append(CaseModifier::apply_case(word.substr(subpos, sublen), case_modifier));
      else
        line.append(word, subpos, sublen);

      previous_token = i;
    }

    return line;
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features) const {
    return tokenize(text, words, features, nullptr);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features,
                           std::unordered_map<std::string, size_t>& alphabets) const {
    return tokenize(text, words, features, &alphabets);
  }

  static bool _endsWithSpace(const std::string &s) {
    return s.length() && s[s.length()-1] == ' ';
  }

  static void _tokenizeByPlaceholder(const std::string &text,
                                     std::vector<AnnotatedToken> &annotated_tokens,
                                     bool preserve_placeholders) {
    size_t q = 0;
    while (1) {
      if (q != 0 && text[q] != ' ')
        annotated_tokens.back().join_right();
      size_t p = text.find(Tokenizer::ph_marker_open, q);
      if (p == std::string::npos) {
        annotated_tokens.emplace_back(text.substr(q));
        break;
      }
      if (p != q)
        annotated_tokens.emplace_back(text.substr(q, p-q));
      if (annotated_tokens.size() && !_endsWithSpace(annotated_tokens.back().str()))
        annotated_tokens.back().join_right();
      q = text.find(Tokenizer::ph_marker_close, p);
      if (q == std::string::npos) {
        annotated_tokens.emplace_back(text.substr(p));
        break;
      }
      q += Tokenizer::ph_marker_close.length();
      annotated_tokens.emplace_back(text.substr(p, q-p));
      if (preserve_placeholders)
        annotated_tokens.back().preserve();
    }
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features,
                           std::unordered_map<std::string, size_t>* alphabets) const
  {
    std::vector<AnnotatedToken> annotated_tokens;
    annotated_tokens.reserve(text.size());

    if (_mode == Mode::None) {
      _tokenizeByPlaceholder(text, annotated_tokens, _preserve_placeholders);
    } else if (_mode == Mode::Space) {
      if (text.empty())
        return;

      std::vector<std::string> chunks = unicode::split_utf8(text, " ");
      for (const auto& chunk: chunks)
      {
        if (chunk.empty())
          continue;

        std::vector<std::string> fields = unicode::split_utf8(chunk, ITokenizer::feature_marker);

        _tokenizeByPlaceholder(fields[0], annotated_tokens, _preserve_placeholders);

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

      bool uppercase = false;
      bool uppercase_sequence = false;
      int state = State::Space;
      int prev_alphabet = -1;

      for (size_t i = 0; i < chars.size(); ++i)
      {
        const std::string& c = chars[i];
        unicode::code_point_t v = code_points[i];
        unicode::code_point_t next_v = i + 1 < code_points.size() ? code_points[i + 1] : 0;
        bool isSeparator = unicode::is_separator(v);

        const bool letter = state & State::Letter;
        const bool space = state & State::Space;
        const bool number = state & State::Number;
        const bool other = state & State::Other;
        const bool placeholder = state & State::Placeholder;

        if (placeholder) {
            if (c == Tokenizer::ph_marker_close) {
              token.append(c);
              if (_preserve_placeholders)
                token.preserve();
              prev_alphabet = placeholder_alphabet;
              state = State::Letter;
            } else {
              if (isSeparator && !_no_substitution) {
                char buffer[10];
                sprintf(buffer, "%04x", v);
                token.append(protected_character + buffer);
              } else {
                token.append(c);
              }
            }
          }
          else if (c == Tokenizer::ph_marker_open) {
            if (!space) {
              AnnotatedToken next_token;
              if ((letter && prev_alphabet != placeholder_alphabet) || number)
                next_token.join_left();
              else
                token.join_right();
              annotated_tokens.emplace_back(std::move(token));
              std::swap(token, next_token);
            } else if (other && token.str().empty()) {
              annotated_tokens.back().join_right();
            }
            token.append(c);
            state = State::Placeholder;
        }
        else if (isSeparator)
        {
          if (!space)
          {
            annotated_tokens.emplace_back(std::move(token));
            token.clear();
          }

          if (v == 0x200D) // Zero-Width joiner.
          {
            if (other || (number && unicode::is_letter(next_v)))
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
              annotated_tokens.emplace_back(std::move(token));
              token.clear();
            }
          }

          uppercase = false;
          uppercase_sequence = false;
          state = State::Space;
        }
        else
        {
          bool cur_letter = false;
          bool cur_number = false;
          // skip special characters and BOM
          if (v > 32 && v != 0xFEFF)
          {
            const std::string& sub_c(!_no_substitution && substitutes.find(c) != substitutes.end() ?
                                     substitutes.at(c) : c);
            cur_letter = unicode::is_letter(v);
            cur_number = unicode::is_number(v);

            int alphabet = get_alphabet_id(v);
            if (alphabets != nullptr)
            {
              if (alphabet >= 0 && cur_letter)
                (*alphabets)[id_to_alphabet(static_cast<Alphabet>(alphabet))]++;
              else
                (*alphabets)[cur_number ? "Numeric" : "Other"]++;
            }

            bool is_mark = unicode::is_mark(v);

            if (is_mark) {
              // if we have a mark, we keep type of previous character
              cur_letter = letter;
              cur_number = number;
            }

            if (_mode == Mode::Conservative)
            {
              if (cur_number
                  || (sub_c == "-" && letter)
                  || (sub_c == "_")
                  || (letter && (sub_c == "." || sub_c == ",") && (unicode::is_number(next_v) || unicode::is_letter(next_v))))
                {
                  cur_letter = true;
                  alphabet = number_alphabet;
                }
            }

            if (cur_letter && _mode != Mode::Char)
            {
              unicode::_type_letter type_letter = unicode::get_case(v);
              bool segment_case = false;
              bool segment_alphabet = false;
              bool segment_alphabet_change = false;
              if ((!letter && !space)
                  || (letter && !is_mark &&
                      ((segment_alphabet = (prev_alphabet == alphabet && is_alphabet_to_segment(alphabet)))
                       || (segment_alphabet_change = (prev_alphabet != alphabet && _segment_alphabet_change))
                       || (prev_alphabet == placeholder_alphabet
                           || (_segment_case && (segment_case = (letter
                               && ((type_letter == unicode::_letter_upper && !uppercase)
                                   || (type_letter == unicode::_letter_lower && uppercase_sequence)))))))))
              {
                token.join_right();
                if (_preserve_segmented_tokens
                    && (segment_case || segment_alphabet || segment_alphabet_change))
                  token.preserve();
                annotated_tokens.emplace_back(std::move(token));
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

              token.append(sub_c);
              state = State::Letter;
              prev_alphabet = alphabet;
            }
            else if (cur_number && _mode != Mode::Char)
            {
              if (letter || (number && _segment_numbers) || (!number && !space))
              {
                AnnotatedToken next_token;
                if (!letter || prev_alphabet == placeholder_alphabet)
                  token.join_right();
                else
                  next_token.join_left();
                if (_preserve_segmented_tokens && number && _segment_numbers)
                  token.preserve();
                annotated_tokens.emplace_back(std::move(token));
                std::swap(token, next_token);
              }
              else if (other)
              {
                annotated_tokens.back().join_right();
              }

              token.append(sub_c);
              uppercase = false;
              uppercase_sequence = false;
              state = State::Number;
            }
            else
            {
              if (!space)
              {
                annotated_tokens.emplace_back(std::move(token));
                token.clear();
                token.join_left();
              }
              else if (other)
              {
                token.clear();
                token.join_left();
              }

              token.append(sub_c);
              annotated_tokens.emplace_back(std::move(token));
              token.clear();
              uppercase = false;
              uppercase_sequence = false;
              state = State::Other | State::Space;
            }
          }
        }
      }

      if (!token.str().empty())
        annotated_tokens.emplace_back(std::move(token));
    }

    if (_case_markup)
    {
      for (auto& token : annotated_tokens)
      {
        if (is_placeholder(token.str()))
          continue;
        auto pair = CaseModifier::extract_case_type(token.str());
        if (pair.second == CaseModifier::Type::Uppercase
            || pair.second == CaseModifier::Type::Capitalized)
        {
          token.set(std::move(pair.first));
          token.set_case(pair.second);
          if (pair.second == CaseModifier::Type::Uppercase)
          {
            token.set_case_region_begin(pair.second);
            token.set_case_region_end(pair.second);
          }
        }
      }
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
    tokens.reserve(annotated_tokens.size());

    for (size_t i = 0; i < annotated_tokens.size(); ++i)
    {
      const auto& token = annotated_tokens[i];
      const auto& str = token.str();

      if (token.begin_case_region())
        tokens.emplace_back(CaseModifier::generate_case_markup_begin(token.get_case_region_begin()));
      else if (token.has_case())
        tokens.emplace_back(CaseModifier::generate_case_markup(token.get_case()));

      if (_joiner_annotate)
      {
        if (token.is_joined_left() && i > 0)
        {
          if (_joiner_new || token.should_preserve())
          {
            tokens.push_back(_joiner);
            if (!str.empty())
              tokens.emplace_back(std::move(str));
          }
          else
            tokens.emplace_back(_joiner + str);
        }
        else if (!str.empty())
          tokens.emplace_back(std::move(str));
        if (token.is_joined_right() && i + 1 < annotated_tokens.size())
        {
          if (_joiner_new || token.should_preserve())
            tokens.push_back(_joiner);
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
          if (!str.empty())
            tokens.emplace_back(std::move(str));
        }
        else if (token.should_preserve())
        {
          tokens.push_back(spacer_marker);
          tokens.emplace_back(std::move(str));
        }
        else
        {
          if (_spacer_new)
          {
            tokens.push_back(spacer_marker);
            tokens.emplace_back(std::move(str));
          }
          else
            tokens.emplace_back(spacer_marker + str);
        }
      }
      else if (!str.empty())
      {
        tokens.emplace_back(std::move(str));
      }

      if (token.end_case_region())
        tokens.emplace_back(CaseModifier::generate_case_markup_end(token.get_case_region_end()));
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

  void Tokenizer::unset_annotate() {
    _joiner_annotate = _spacer_annotate = false;
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

  Tokenizer& Tokenizer::set_sp_model(const std::string& model_path, bool cache_model)
  {
#ifdef WITH_SP
    if (_mode == Mode::None && !_joiner_annotate && !_spacer_annotate)
      _spacer_annotate = true;
    return this->set_subword_encoder_model<SentencePiece>(model_path, cache_model);
#else
    throw std::runtime_error("The Tokenizer was not built with SentencePiece support");
#endif
  }

  bool Tokenizer::add_alphabet_to_segment(const std::string& alphabet)
  {
    if (!onmt::alphabet_is_supported(alphabet))
      return false;
    _segment_alphabet.insert(static_cast<int>(alphabet_to_id(alphabet)));
    return true;
  }

  bool Tokenizer::is_alphabet_to_segment(const std::string& alphabet) const
  {
    return _segment_alphabet.count(static_cast<int>(alphabet_to_id(alphabet))) > 0;
  }

  bool Tokenizer::is_alphabet_to_segment(int alphabet) const
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
    return (word.length() >= marker.length() && word.compare(0, marker.length(), marker) == 0);
  }

  bool Tokenizer::has_right_marker(const std::string& word, const std::string& marker) const
  {
    return (word.length() >= marker.length()
            && word.compare(word.length() - marker.length(), marker.length(), marker) == 0);
  }

  bool Tokenizer::is_placeholder(const std::string& str)
  {
    size_t ph_begin = str.find(ph_marker_open);
    if (ph_begin == std::string::npos)
      return false;
    size_t min_ph_end = ph_begin + ph_marker_open.length() + 1;
    return str.find(ph_marker_close, min_ph_end) != std::string::npos;
  }

}
