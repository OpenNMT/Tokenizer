#include "onmt/Tokenizer.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>

#include "onmt/BPE.h"
#include "onmt/SentencePiece.h"
#include "onmt/unicode/Unicode.h"
#include "Casing.h"
#include "Utils.h"

namespace onmt
{

  void set_random_seed(const unsigned int seed)
  {
    set_random_generator_seed(seed);
  }

  const std::string Tokenizer::joiner_marker("￭");
  const std::string Tokenizer::spacer_marker("▁");
  const std::string Tokenizer::ph_marker_open = "｟";
  const std::string Tokenizer::ph_marker_close = "｠";
  static const std::vector<std::string> exclude_combining{Tokenizer::ph_marker_close};

  const std::unordered_map<std::string, Tokenizer::Mode> Tokenizer::mapMode = {
    { "aggressive", Tokenizer::Mode::Aggressive },
    { "conservative", Tokenizer::Mode::Conservative },
    { "space", Tokenizer::Mode::Space },
    { "char", Tokenizer::Mode::Char },
    { "none", Tokenizer::Mode::None }
  };

  Tokenizer::Mode Tokenizer::str_to_mode(const std::string& mode) {
    auto it = mapMode.find(mode);
    if (it == mapMode.end())
      throw std::invalid_argument("invalid tokenization mode: " + mode);
    return it->second;
  }

  enum State
  {
    Letter = 1 << 0,
    Number = 1 << 1,
    Space = 1 << 2,
    Other = 1 << 3,
    Placeholder = 1 << 4
  };

  static const std::string protected_character = "％";
  static const std::vector<std::string> special_chars = {"▁", "￭", "￨", "％", "＃", "："};
  static const std::vector<std::string> substitutes = {"_", "■", "│", "%", "#", ":"};

  static const std::string& normalize_character(const std::string& c) {
    auto it = std::find(special_chars.begin(), special_chars.end(), c);
    if (it != special_chars.end())
      return substitutes[std::distance(special_chars.begin(), it)];
    return c;
  }

  static void annotate_case(std::vector<Token>& annotated_tokens)
  {
    for (auto& token : annotated_tokens)
    {
      if (token.is_placeholder())
        continue;
      std::tie(token.surface, token.casing) = lowercase_token(token.surface);
    }
  }

  template <typename T>
  std::string int_to_hex(T i, int width = 4)
  {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(width) << std::hex << i;
    return stream.str();
  }

  Tokenizer::Tokenizer(Mode mode,
                       int flags,
                       const std::string& model_path,
                       const std::string& joiner,
                       const std::string& vocab_path,
                       int vocab_threshold)
    : _mode(mode)
    , _joiner(joiner)
  {
    read_flags(flags);
    if (!model_path.empty())
    {
      if (flags & Flags::SentencePieceModel)
        set_subword_encoder(std::make_shared<SentencePiece>(model_path));
      else
        set_subword_encoder(std::make_shared<BPE>(model_path));

      if (!vocab_path.empty())
        _subword_encoder->load_vocabulary(vocab_path, vocab_threshold);
    }
  }

  Tokenizer::Tokenizer(Mode mode,
                       SubwordEncoder* subword_encoder,
                       int flags,
                       const std::string& joiner)
    : _mode(mode)
    , _joiner(joiner)
  {
    read_flags(flags);
    set_subword_encoder(std::shared_ptr<SubwordEncoder>(subword_encoder));
  }

  Tokenizer::Tokenizer(const std::string& sp_model_path,
                       int sp_nbest_size,
                       float sp_alpha,
                       Mode mode,
                       int flags,
                       const std::string& joiner)
    : _mode(mode)
    , _joiner(joiner)
  {
    read_flags(flags);
    set_subword_encoder(std::make_shared<SentencePiece>(sp_model_path, sp_nbest_size, sp_alpha));
  }

  void Tokenizer::read_flags(int flags)
  {
    _case_feature = flags & Flags::CaseFeature;
    _case_markup = flags & Flags::CaseMarkup;
    _soft_case_regions = flags & Flags::SoftCaseRegions;
    _joiner_annotate = flags & Flags::JoinerAnnotate;
    _joiner_new = flags & Flags::JoinerNew;
    _with_separators = flags & Flags::WithSeparators;
    _segment_case = (flags & Flags::SegmentCase) | (flags & Flags::CaseMarkup);
    _segment_numbers = flags & Flags::SegmentNumbers;
    _segment_alphabet_change = flags & Flags::SegmentAlphabetChange;
    _no_substitution = flags & Flags::NoSubstitution;
    _spacer_annotate = flags & Flags::SpacerAnnotate;
    _spacer_new = flags & Flags::SpacerNew;
    _preserve_placeholders = flags & Flags::PreservePlaceholders;
    _preserve_segmented_tokens = flags & Flags::PreserveSegmentedTokens;
    _support_prior_joiners = flags & Flags::SupportPriorJoiners;

    if ((flags & Flags::CacheBPEModel) | (flags & Flags::CacheModel))
      throw std::invalid_argument("Subword model caching is deprecated and should be handled in the client side");
    if (_case_feature && _case_markup)
      throw std::invalid_argument("case_feature and case_markup can't be set at the same time");
    if (_joiner_annotate && _spacer_annotate)
      throw std::invalid_argument("joiner_annotate and spacer_annotate can't be set at the same time");
    if (_spacer_new && !_spacer_annotate)
      throw std::invalid_argument("spacer_new requires spacer_annotate");
    if (_joiner_new && !_joiner_annotate)
      throw std::invalid_argument("joiner_new requires joiner_annotate");
    if (_support_prior_joiners && unicode::utf8len(_joiner) != 1)
      throw std::invalid_argument("support_prior_joiners does not support multi-character joiners");
  }

  std::string Tokenizer::detokenize(const std::vector<std::string>& words,
                                    const std::vector<std::vector<std::string> >& features) const
  {
    return detokenize(words, features, nullptr);
  }

  std::string
  Tokenizer::detokenize(const std::vector<std::string>& words,
                        const std::vector<std::vector<std::string> >& features,
                        Ranges& ranges, bool merge_ranges) const
  {
    return detokenize(words, features, &ranges, merge_ranges);
  }

  std::string Tokenizer::detokenize(const std::vector<Token>& tokens) const
  {
    return detokenize(tokens, nullptr);
  }

  std::string Tokenizer::detokenize(const std::vector<Token>& tokens,
                                    Ranges& ranges, bool merge_ranges) const
  {
    return detokenize(tokens, &ranges, merge_ranges);
  }

  static Ranges merge_consecutive_ranges(const std::string& text, const Ranges& ranges)
  {
    // We do not want to merge ranges that represent different tokens. To do so, we run a
    // basic tokenization on consecutive ranges. If they are tokenized, we do not
    // merge the ranges.
    Tokenizer tokenizer(Tokenizer::Mode::Conservative);
    std::vector<std::string> tokens;
    std::string bridge;

    Ranges merged_ranges;
    std::vector<size_t> current_token;
    int start = 0;
    int end = -1;
    for (auto it = ranges.begin(); it != ranges.end(); ++it)
    {
      const auto& index = it->first;
      const auto& range = it->second;
      bool split = static_cast<int>(range.first) != end + 1;
      if (!split && it != ranges.begin())
      {
        const auto& prev_range = std::prev(it)->second;
        auto prev_length = prev_range.second - prev_range.first + 1;
        auto curr_length = range.second - range.first + 1;
        bridge.assign(text, prev_range.first, prev_length + curr_length);
        tokens.clear();
        tokenizer.tokenize(bridge, tokens);
        split = tokens.size() > 1;
      }

      if (split)
      {
        for (size_t id : current_token)
          merged_ranges.emplace(std::piecewise_construct,
                                std::forward_as_tuple(id),
                                std::forward_as_tuple(start, end));
        current_token.clear();
        start = range.first;
      }

      end = range.second;
      current_token.push_back(index);
    }

    if (!current_token.empty())
    {
      for (size_t id : current_token)
        merged_ranges.emplace(std::piecewise_construct,
                              std::forward_as_tuple(id),
                              std::forward_as_tuple(start, end));
    }

    return merged_ranges;
  }

  std::string Tokenizer::detokenize(const std::vector<Token>& tokens,
                                    Ranges* ranges,
                                    bool merge_ranges,
                                    const std::vector<size_t>* index_map) const
  {
    std::string line;
    line.reserve(tokens.size() * 10);

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      const auto& token = tokens[i];
      if (i > 0 && !tokens[i - 1].join_right && !token.join_left)
        line += ' ';

      std::string prep_word = token.surface;

      if (!token.is_placeholder())
      {
        if (token.casing != Casing::None)
          prep_word = restore_token_casing(prep_word, token.casing);

        size_t p = prep_word.find(protected_character, 0);
        while (p != std::string::npos && p+protected_character.size()+4 < prep_word.size()) {
          std::string code = prep_word.substr(p+protected_character.size(), 4);
          int v;
          if (sscanf(code.c_str(), "%x", &v) == 1)
            prep_word.replace(p, protected_character.size()+4, unicode::cp_to_utf8(v));
          p = prep_word.find(protected_character, p+protected_character.size());
        }
      }

      if (!prep_word.empty())
      {
        if (ranges)
          ranges->emplace(std::piecewise_construct,
                          std::forward_as_tuple(index_map ? index_map->at(i) : i),
                          std::forward_as_tuple(line.size(), line.size() + prep_word.size() - 1));

        line.append(prep_word);
      }
    }

    if (ranges && merge_ranges)
      *ranges = merge_consecutive_ranges(line, *ranges);

    return line;
  }

  Token Tokenizer::annotate_token(const std::string& word) const
  {
    Token token;
    size_t subpos = 0;
    size_t sublen = word.size();

    if (_spacer_annotate)
    {
      if (starts_with(word, spacer_marker))
      {
        subpos += spacer_marker.length();
        sublen -= spacer_marker.length();
      }
      else
        token.join_left = true;
    }
    else
    {
      if (ends_with(word, _joiner))
      {
        token.join_right = true;
        sublen -= _joiner.length();
      }
      if (starts_with(word, _joiner))
      {
        token.join_left = true;
        subpos += _joiner.length();
        sublen -= _joiner.length();
      }
    }

    token.surface = word.substr(subpos, sublen);
    return token;
  }

  void Tokenizer::annotate_tokens(const std::vector<std::string>& words,
                                  const std::vector<std::vector<std::string>>& features,
                                  std::vector<Token>& tokens) const
  {
    if (_subword_encoder)
      tokenize(detokenize(words, features), tokens);
    else
      parse_tokens(words, features, tokens);
  }

  void Tokenizer::parse_tokens(const std::vector<std::string>& words,
                               const std::vector<std::vector<std::string>>& features,
                               std::vector<Token>& tokens,
                               std::vector<size_t>* index_map) const
  {
    tokens.reserve(words.size());
    if (index_map)
      index_map->reserve(words.size());
    Casing case_region = Casing::None;
    Casing case_modifier = Casing::None;

    for (size_t i = 0; i < words.size(); ++i)
    {
      if (words[i].empty())
        continue;

      size_t features_offset = 0;
      if (_case_feature)
      {
        if (features.empty())
          throw std::runtime_error("Missing case feature");
        case_modifier = char_to_casing(features[0][i][0]);
        features_offset = 1;
      }
      else
      {
        auto case_markup = read_case_markup(words[i]);
        switch (case_markup)
        {
        case CaseMarkupType::RegionBegin:
          case_region = get_casing_from_markup(words[i]);
          case_modifier = Casing::None;
          continue;
        case CaseMarkupType::RegionEnd:
          case_region = Casing::None;
          case_modifier = Casing::None;
          continue;
        case CaseMarkupType::Modifier:
          case_modifier = get_casing_from_markup(words[i]);
          continue;
        default:
          case_modifier = (case_modifier != Casing::None ? case_modifier : case_region);
          break;
        }
      }

      Token token = annotate_token(words[i]);
      token.casing = case_modifier;
      if (!features.empty())
      {
        for (size_t j = features_offset; j < features.size(); ++j)
          token.append_feature(features[j][i]);
      }
      // Forward the case modifier if the current token is a joiner or spacer.
      if (!token.empty())
        case_modifier = Casing::None;

      tokens.emplace_back(std::move(token));
      if (index_map)
        index_map->push_back(i);
    }
  }

  std::string Tokenizer::detokenize(const std::vector<std::string>& words,
                                    const std::vector<std::vector<std::string> >& features,
                                    Ranges* ranges, bool merge_ranges) const
  {
    std::vector<Token> tokens;
    std::vector<size_t> index_map;
    parse_tokens(words, features, tokens, &index_map);
    return detokenize(tokens, ranges, merge_ranges, &index_map);
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

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<Token>& annotated_tokens) const {
    return tokenize(text, annotated_tokens, nullptr);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features,
                           std::unordered_map<std::string, size_t>* alphabets) const
  {
    std::vector<Token> annotated_tokens;
    tokenize(text, annotated_tokens, alphabets);
    finalize_tokens(annotated_tokens, words, features);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<Token>& annotated_tokens,
                           std::unordered_map<std::string, size_t>* alphabets) const
  {
    if (text.empty())
      return;

    annotated_tokens.reserve(text.size());

    switch (_mode)
    {
    case Mode::None:
      tokenize_on_placeholders(text, annotated_tokens);
      break;
    case Mode::Space:
      tokenize_on_spaces(text, annotated_tokens);
      break;
    default:
      tokenize_text(text, annotated_tokens, alphabets);
      break;
    }

    if (_case_markup || _case_feature)
      annotate_case(annotated_tokens);
    if (_subword_encoder)
      annotated_tokens = _subword_encoder->encode_and_annotate(annotated_tokens);
  }

  void Tokenizer::tokenize_on_placeholders(const std::string& text,
                                           std::vector<Token>& tokens) const
  {
    // Split on characters.
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points_main;
    std::vector<std::vector<unicode::code_point_t>> code_points_combining;
    unicode::explode_utf8_with_marks(text, chars, code_points_main, code_points_combining);

    Token token;  // Accumulate characters in this.
    bool in_placeholder = false;

    for (size_t i = 0; i < chars.size(); ++i)
    {
      const auto& c = chars[i];

      if (!in_placeholder)
      {
        if (_support_prior_joiners && c == _joiner)
        {
          // Mark joint but discard character.
          if (token.empty())
            token.join_left = true;
          else
            token.join_right = true;
        }
        else if (c == ph_marker_open)
        {
          if (!token.empty())
          {
            // Flush accumulated token and mark joint if it did not finish by a separator.
            if (i > 0 && !unicode::is_separator(code_points_main[i - 1]))
              token.join_right = true;
            if (_preserve_segmented_tokens)
              token.preserve = true;
            tokens.emplace_back(std::move(token));
            token = Token();
          }

          token.append(c);
          in_placeholder = true;
        }
        else
        {
          // Normalize character for consistency with other tokenization modes.
          token.append(_no_substitution ? c : normalize_character(c));
        }
      }
      else  // In a placeholder.
      {
        token.append(c);  // Do not normalize character inside placeholders.
        if (c == ph_marker_close)
        {
          // Flush accumulated placeholder and mark joint if the next character is not a separator.
          // No need to check for emptiness as in_placeholder == true means at least the opening
          // character was accumulated.
          if (i + 1 < chars.size() && !unicode::is_separator(code_points_main[i + 1]))
            token.join_right = true;
          if (_preserve_placeholders || _preserve_segmented_tokens)
            token.preserve = true;
          tokens.emplace_back(std::move(token));
          token = Token();
          in_placeholder = false;
        }
      }
    }

    // Flush remaining token.
    if (!token.empty())
      tokens.emplace_back(std::move(token));
  }

  void Tokenizer::tokenize_on_spaces(const std::string& text,
                                     std::vector<Token>& annotated_tokens) const
  {
    {
      std::vector<std::string> chunks = split_string(text, " ");
      for (auto& chunk: chunks)
      {
        if (chunk.empty())
          continue;

        std::vector<std::string> fields = split_string(chunk, ITokenizer::feature_marker);
        auto& token = fields[0];

        std::vector<Token> sub_tokens;
        tokenize_on_placeholders(token, sub_tokens);

        for (size_t i = 1; i < fields.size(); ++i)
        {
          // Replicate the features to each sub token.
          for (auto& sub_token : sub_tokens)
            sub_token.append_feature(fields[i]);
        }

        annotated_tokens.insert(annotated_tokens.end(), sub_tokens.begin(), sub_tokens.end());
      }
    }
  }

  void Tokenizer::tokenize_text(const std::string& text,
                                std::vector<Token>& annotated_tokens,
                                std::unordered_map<std::string, size_t>* alphabets) const
  {
    // TODO: this method has grown big and is hard to follow. It should be refactored into
    // smaller pieces to clarify its logic.

    {
      std::vector<std::string> chars;
      std::vector<unicode::code_point_t> code_points_main;
      std::vector<std::vector<unicode::code_point_t>> code_points_combining;

      unicode::explode_utf8_with_marks(text,
                                       chars,
                                       &code_points_main,
                                       &code_points_combining,
                                       &exclude_combining);

      Token token;

      bool uppercase = false;
      bool uppercase_sequence = false;
      int state = State::Space;
      int prev_alphabet = -1;

      for (size_t i = 0; i < chars.size(); ++i)
      {
        const bool letter = state & State::Letter;
        const bool space = state & State::Space;
        const bool number = state & State::Number;
        const bool other = state & State::Other;
        const bool placeholder = state & State::Placeholder;

        const std::string& c = chars[i];
        if (_support_prior_joiners && c == _joiner) {
          /* it is either after a space, in that case it annotates the following word,
             or a closed token (other & space), or a unclosed token - in that case it is a right joiner.
            */
          if (other) {
            annotated_tokens.back().join_right = true;
            continue;
          }
          else if (space) {
            token.join_left = true;
            continue;
          } else {
            token.join_right = true;
            annotated_tokens.emplace_back(std::move(token));
            token = Token();
            state = State::Space;
            continue;
          }
        }
        unicode::code_point_t v = code_points_main[i];
        unicode::code_point_t next_v = i + 1 < code_points_main.size() ? code_points_main[i + 1] : 0;
        bool is_separator = unicode::is_separator(v) && code_points_combining[i].size() == 0;

        if (placeholder) {
          if (c == Tokenizer::ph_marker_close) {
            token.append(c);
            if (_preserve_placeholders)
              token.preserve = true;
            prev_alphabet = placeholder_alphabet;
            state = State::Letter;
          } else {
            if (is_separator && !_no_substitution) {
              token.append(protected_character + int_to_hex(v));
            } else {
              token.append(c);
            }
          }
        }
        else if (c == Tokenizer::ph_marker_open) {
          if (!space) {
            Token next_token;
            if ((letter && prev_alphabet != placeholder_alphabet) || number)
              next_token.join_left = true;
            else
              token.join_right = true;
            annotated_tokens.emplace_back(std::move(token));
            std::swap(token, next_token);
          } else if (other && token.empty()) {
            annotated_tokens.back().join_right = true;
          }
          token.append(c);
          state = State::Placeholder;
        }
        else if (is_separator)
        {
          if (!space)
          {
            annotated_tokens.emplace_back(std::move(token));
            token = Token();
          }

          if (v == 0x200D) // Zero-Width joiner.
          {
            if (other || (number && unicode::is_letter(next_v)))
              annotated_tokens.back().join_right = true;
            else
            {
              token = Token();
              token.join_left = true;
            }
          }
          else if (_with_separators)
          {
            token.append(c);
            if (!unicode::is_separator(next_v))
            {
              annotated_tokens.emplace_back(std::move(token));
              token = Token();
            }
          }

          uppercase = false;
          uppercase_sequence = false;
          state = State::Space;
        }
        else
        {
          // skip special characters and BOM
          if (v >= 32 && v != 0xFEFF)
          {
            const std::string& sub_c(_no_substitution ? c : normalize_character(c));
            bool is_letter = unicode::is_letter(v);
            bool is_number = !is_letter && unicode::is_number(v);
            int alphabet = unicode::get_script(v);

            if (alphabets != nullptr)
            {
              if (alphabet >= 0 && is_letter)
                (*alphabets)[unicode::get_script_name(alphabet)]++;
              else
                (*alphabets)[is_number ? "Numeric" : "Other"]++;
            }

            if (_mode == Mode::Conservative)
            {
              if (is_number
                  || (sub_c == "-" && letter)
                  || (sub_c == "_")
                  || (letter && (sub_c == "." || sub_c == ",") && (unicode::is_number(next_v) || unicode::is_letter(next_v))))
                {
                  is_letter = true;
                  alphabet = number_alphabet;
                }
            }

            if (is_letter && _mode != Mode::Char)
            {
              const unicode::CaseType case_type = unicode::get_case_v2(v);
              bool segment_case = false;
              bool segment_alphabet = false;
              bool segment_alphabet_change = false;
              if ((!letter && !space)
                  || (letter &&
                      ((segment_alphabet = (prev_alphabet == alphabet && is_alphabet_to_segment(alphabet)))
                       || (segment_alphabet_change = (prev_alphabet != alphabet && _segment_alphabet_change))
                       || (prev_alphabet == placeholder_alphabet
                           || (_segment_case && (segment_case = (letter
                               && ((case_type == unicode::CaseType::Upper && !uppercase)
                                   || (case_type == unicode::CaseType::Lower && uppercase_sequence)))))))))
              {
                token.join_right = true;
                if (_preserve_segmented_tokens
                    && (segment_case || segment_alphabet || segment_alphabet_change))
                  token.preserve = true;
                annotated_tokens.emplace_back(std::move(token));
                token = Token();
                uppercase = (case_type == unicode::CaseType::Upper);
                uppercase_sequence = false;
              }
              else if (other && token.empty())
              {
                annotated_tokens.back().join_right = true;
                uppercase = (case_type == unicode::CaseType::Upper);
                uppercase_sequence = false;
              } else {
                uppercase_sequence = (case_type == unicode::CaseType::Upper) & uppercase;
                uppercase = (case_type == unicode::CaseType::Upper);
              }

              token.append(sub_c);
              state = State::Letter;
              prev_alphabet = alphabet;
            }
            else if (is_number && _mode != Mode::Char)
            {
              if (letter || (number && _segment_numbers) || (!number && !space))
              {
                Token next_token;
                if (!letter || prev_alphabet == placeholder_alphabet)
                  token.join_right = true;
                else
                  next_token.join_left = true;
                if (_preserve_segmented_tokens && number && _segment_numbers)
                  token.preserve = true;
                annotated_tokens.emplace_back(std::move(token));
                std::swap(token, next_token);
              }
              else if (other)
              {
                annotated_tokens.back().join_right = true;
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
                token = Token();
                token.join_left = true;
              }
              else if (other)
              {
                token = Token();
                token.join_left = true;
              }

              if (sub_c[0] == ' ' && !_no_substitution)
                token.append(protected_character + "0020" + sub_c.substr(1));
              else
                token.append(sub_c);

              annotated_tokens.emplace_back(std::move(token));
              token = Token();
              uppercase = false;
              uppercase_sequence = false;
              state = State::Other | State::Space;
            }
          }
        }
      }

      if (!token.empty())
        annotated_tokens.emplace_back(std::move(token));
    }
  }

  static void add_final_token(std::vector<std::string>& tokens,
                              std::vector<std::vector<std::string>>& features,
                              bool case_feature,
                              std::string token,
                              Casing casing = Casing::None)
  {
    if (token.empty())
      return;
    tokens.emplace_back(std::move(token));
    if (case_feature)
      features.back().emplace_back(1, casing_to_char(casing));
  }

  void Tokenizer::finalize_tokens(const std::vector<Token>& annotated_tokens,
                                  std::vector<std::string>& tokens,
                                  std::vector<std::vector<std::string>>& features) const
  {
    tokens.reserve(annotated_tokens.size());
    size_t num_features = 0;
    if (annotated_tokens.size() > 0 && annotated_tokens[0].has_features())
      num_features = annotated_tokens[0].features.size();
    if (_case_feature)
      num_features += 1;

    for (size_t i = 0; i < num_features; ++i)
    {
      features.emplace_back(0);
      features.back().reserve(annotated_tokens.size());
    }

    std::vector<TokenCaseMarkup> case_markups;
    if (_case_markup)
      case_markups = get_case_markups(annotated_tokens, _soft_case_regions);

    for (size_t i = 0; i < annotated_tokens.size(); ++i)
    {
      const auto& token = annotated_tokens[i];
      const auto& str = token.surface;
      const auto casing = token.casing;

      if (token.has_features())
      {
        const auto& token_features = token.features;
        for (size_t j = 0; j < token_features.size(); ++j)
          features[j].push_back(token_features[j]);
      }

      if (_case_markup && case_markups[i].prefix != CaseMarkupType::None)
        tokens.emplace_back(write_case_markup(case_markups[i].prefix, case_markups[i].casing));

      const std::string* prefix = nullptr;
      const std::string* suffix = nullptr;
      bool attach = !token.preserve;

      if (_joiner_annotate)
      {
        if (token.join_left && i > 0)
          prefix = &_joiner;
        if (token.join_right && i + 1 < annotated_tokens.size())
          suffix = &_joiner;
        if (token.spacer)
          attach = true;  // Ignore preserve flag for spacers in joiner mode.
        attach = attach && !_joiner_new;
      }
      else if (_spacer_annotate)
      {
        if ((i == 0 && token.spacer)
            || (i > 0 && !token.join_left && !annotated_tokens[i - 1].join_right))
          prefix = &spacer_marker;
        attach = attach && !_spacer_new;
      }

      if (!prefix && !suffix)
        add_final_token(tokens, features, _case_feature, str, casing);
      else if (attach)
      {
        std::string final_token = (prefix ? *prefix : "") + str + (suffix ? *suffix : "");
        add_final_token(tokens, features, _case_feature, std::move(final_token), casing);
      }
      else
      {
        if (prefix)
          add_final_token(tokens, features, _case_feature, *prefix);
        add_final_token(tokens, features, _case_feature, str, casing);
        if (suffix)
          add_final_token(tokens, features, _case_feature, *suffix);
      }

      if (_case_markup && case_markups[i].suffix != CaseMarkupType::None)
        tokens.emplace_back(write_case_markup(case_markups[i].suffix, case_markups[i].casing));
    }
  }

  Tokenizer& Tokenizer::set_joiner(const std::string& joiner)
  {
    _joiner = joiner;
    return *this;
  }

  void Tokenizer::unset_annotate() {
    _joiner_annotate = _spacer_annotate = false;
  }

  void Tokenizer::set_subword_encoder(const std::shared_ptr<SubwordEncoder>& subword_encoder)
  {
    _subword_encoder = subword_encoder;

    // TODO: clean this up, declare a base method "declare_tokenization_options".
    auto* encoder = _subword_encoder.get();
    auto* sp = encoder ? dynamic_cast<SentencePiece*>(encoder) : nullptr;
    auto* bpe = encoder && !sp ? dynamic_cast<BPE*>(encoder) : nullptr;

    if (sp)
    {
      // Maybe enable SentencePiece compatibility mode.
      if (_mode == Mode::None
          && !_joiner_annotate
          && !_spacer_annotate)
      {
        _spacer_annotate = true;
        _no_substitution = true;
      }
    }
    else if (bpe)
    {
      bpe->set_joiner(_joiner);
    }
  }

  bool Tokenizer::add_alphabet_to_segment(const std::string& alphabet)
  {
    const int script_code = unicode::get_script_code(alphabet.c_str());
    if (script_code < 0)
      return false;
    _segment_alphabet.insert(script_code);
    return true;
  }

  bool Tokenizer::is_alphabet_to_segment(const std::string& alphabet) const
  {
    return _segment_alphabet.count(unicode::get_script_code(alphabet.c_str())) > 0;
  }

  bool Tokenizer::is_alphabet_to_segment(int alphabet) const
  {
    auto it = _segment_alphabet.find(alphabet);
    return it != _segment_alphabet.end();
  }

  bool Tokenizer::is_placeholder(const std::string& str)
  {
    return ::onmt::is_placeholder(str);
  }

}
