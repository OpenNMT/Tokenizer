#include "onmt/Tokenizer.h"

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

  const std::string Tokenizer::joiner_marker = "￭";
  const std::string Tokenizer::spacer_marker = "▁";
  const std::string Tokenizer::ph_marker_open = "｟";
  const std::string Tokenizer::ph_marker_close = "｠";
  static const unicode::code_point_t ph_marker_open_cp = 0xFF5F;
  static const unicode::code_point_t ph_marker_close_cp = 0xFF60;
  static const std::string protected_character = "％";
  static const std::vector<unicode::code_point_t> exclude_combining{ph_marker_close_cp};
  static const std::vector<std::pair<unicode::code_point_t, std::string>> substitutes = {
    {0x2581 /* ▁ */, "_"},
    {0xFFED /* ￭ */, "■"},
    {0xFFE8 /* ￨ */, "│"},
    {0xFF05 /* ％ */, "%"},
    {0xFF03 /* ＃ */, "#"},
    {0xFF1A /* ： */, ":"},
  };

  static const int placeholder_alphabet = -2;
  static const int number_alphabet = -3;

  Tokenizer::Mode Tokenizer::str_to_mode(const std::string& mode) {
    if (mode == "conservative")
      return Mode::Conservative;
    if (mode == "aggressive")
      return Mode::Aggressive;
    if (mode == "none")
      return Mode::None;
    if (mode == "space")
      return Mode::Space;
    if (mode == "char")
      return Mode::Char;
    throw std::invalid_argument("invalid tokenization mode: " + mode);
  }

  enum State
  {
    Letter = 1 << 0,
    Number = 1 << 1,
    Space = 1 << 2,
    Other = 1 << 3,
    Placeholder = 1 << 4
  };

  static inline const std::string& normalize_character(const std::string& c,
                                                       const unicode::code_point_t v)
  {
    for (const auto& pair : substitutes)
    {
      if (pair.first == v)
        return pair.second;
    }
    return c;
  }

  Tokenizer::Options::Options(Mode mode_, int flags, const std::string& joiner_)
  {
    mode = mode_;
    joiner = joiner_;
    case_feature = flags & Flags::CaseFeature;
    case_markup = flags & Flags::CaseMarkup;
    soft_case_regions = flags & Flags::SoftCaseRegions;
    joiner_annotate = flags & Flags::JoinerAnnotate;
    joiner_new = flags & Flags::JoinerNew;
    with_separators = flags & Flags::WithSeparators;
    segment_case = flags & Flags::SegmentCase;
    segment_numbers = flags & Flags::SegmentNumbers;
    segment_alphabet_change = flags & Flags::SegmentAlphabetChange;
    no_substitution = flags & Flags::NoSubstitution;
    spacer_annotate = flags & Flags::SpacerAnnotate;
    spacer_new = flags & Flags::SpacerNew;
    preserve_placeholders = flags & Flags::PreservePlaceholders;
    preserve_segmented_tokens = flags & Flags::PreserveSegmentedTokens;
    support_prior_joiners = flags & Flags::SupportPriorJoiners;

    if ((flags & Flags::CacheBPEModel) | (flags & Flags::CacheModel))
      throw std::invalid_argument("Subword model caching is deprecated and should be handled in the client side");
  }

  void Tokenizer::Options::validate()
  {
    // Set default options.
    if (joiner.empty())
      joiner = joiner_marker;
    if (case_markup)
      segment_case = true;

    // Check options consistency.
    if (case_feature && case_markup)
      throw std::invalid_argument("case_feature and case_markup can't be set at the same time");
    if (joiner_annotate && spacer_annotate)
      throw std::invalid_argument("joiner_annotate and spacer_annotate can't be set at the same time");
    if (spacer_new && !spacer_annotate)
      throw std::invalid_argument("spacer_new requires spacer_annotate");
    if (joiner_new && !joiner_annotate)
      throw std::invalid_argument("joiner_new requires joiner_annotate");
    if (support_prior_joiners && unicode::utf8len(joiner) != 1)
      throw std::invalid_argument("support_prior_joiners does not support multi-character joiners");

    for (const std::string& alphabet : segment_alphabet)
    {
      if (!add_alphabet_to_segment(alphabet))
        throw std::invalid_argument("invalid Unicode script: " + alphabet);
    }
  }

  bool Tokenizer::Options::add_alphabet_to_segment(const std::string& alphabet)
  {
    const int code = unicode::get_script_code(alphabet.c_str());
    if (code < 0)
      return false;
    segment_alphabet_codes.emplace(code);
    return true;
  }


  Tokenizer::Tokenizer(Options options,
                       const std::shared_ptr<const SubwordEncoder>& subword_encoder)
    : _options(std::move(options))
  {
    _options.validate();
    set_subword_encoder(subword_encoder);
  }

  Tokenizer::Tokenizer(Mode mode,
                       int flags,
                       const std::string& model_path,
                       const std::string& joiner,
                       const std::string& vocab_path,
                       int vocab_threshold)
    : _options(mode, flags, joiner)
  {
    _options.validate();
    if (!model_path.empty())
    {
      SubwordEncoder* subword_encoder = nullptr;
      if (flags & Flags::SentencePieceModel)
        subword_encoder = new SentencePiece(model_path);
      else
        subword_encoder = new BPE(model_path);

      if (!vocab_path.empty())
        subword_encoder->load_vocabulary(vocab_path, vocab_threshold, &_options);

      set_subword_encoder(std::shared_ptr<const SubwordEncoder>(subword_encoder));
    }
  }

  Tokenizer::Tokenizer(Mode mode,
                       const SubwordEncoder* subword_encoder,
                       int flags,
                       const std::string& joiner)
    : _options(mode, flags, joiner)
  {
    _options.validate();
    set_subword_encoder(std::shared_ptr<const SubwordEncoder>(subword_encoder));
  }

  Tokenizer::Tokenizer(const std::string& sp_model_path,
                       int sp_nbest_size,
                       float sp_alpha,
                       Mode mode,
                       int flags,
                       const std::string& joiner)
    : _options(mode, flags, joiner)
  {
    _options.validate();
    set_subword_encoder(std::make_shared<const SentencePiece>(sp_model_path, sp_nbest_size, sp_alpha));
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

    if (_options.spacer_annotate)
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
      if (ends_with(word, _options.joiner))
      {
        token.join_right = true;
        sublen -= _options.joiner.length();
      }
      if (starts_with(word, _options.joiner))
      {
        token.join_left = true;
        subpos += _options.joiner.length();
        sublen -= _options.joiner.length();
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
      if (_options.case_feature)
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

    switch (_options.mode)
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

    if (_options.case_markup || _options.case_feature)
    {
      for (auto& token : annotated_tokens)
        token.lowercase();
    }

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
      const auto v = code_points_main[i];

      if (!in_placeholder)
      {
        if (_options.support_prior_joiners && c == _options.joiner)
        {
          // Mark joint but discard character.
          if (token.empty())
            token.join_left = true;
          else
            token.join_right = true;
        }
        else if (v == ph_marker_open_cp)
        {
          if (!token.empty())
          {
            // Flush accumulated token and mark joint if it did not finish by a separator.
            if (i > 0 && !unicode::is_separator(code_points_main[i - 1]))
              token.join_right = true;
            if (_options.preserve_segmented_tokens)
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
          token.append(_options.no_substitution ? c : normalize_character(c, v));
        }
      }
      else  // In a placeholder.
      {
        token.append(c);  // Do not normalize character inside placeholders.
        if (v == ph_marker_close_cp)
        {
          // Flush accumulated placeholder and mark joint if the next character is not a separator.
          // No need to check for emptiness as in_placeholder == true means at least the opening
          // character was accumulated.
          if (i + 1 < chars.size() && !unicode::is_separator(code_points_main[i + 1]))
            token.join_right = true;
          if (_options.preserve_placeholders || _options.preserve_segmented_tokens)
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
        if (_options.support_prior_joiners && c == _options.joiner) {
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
          if (v == ph_marker_close_cp) {
            token.append(c);
            if (_options.preserve_placeholders)
              token.preserve = true;
            prev_alphabet = placeholder_alphabet;
            state = State::Letter;
          } else {
            if (is_separator && !_options.no_substitution) {
              token.append(protected_character + int_to_hex(v));
            } else {
              token.append(c);
            }
          }
        }
        else if (v == ph_marker_open_cp) {
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
          else if (_options.with_separators)
          {
            token.append(c);
            if (!unicode::is_separator(next_v))
            {
              annotated_tokens.emplace_back(std::move(token));
              token = Token();
            }
          }

          state = State::Space;
        }
        else
        {
          // skip special characters and BOM
          if (v >= 32 && v != 0xFEFF)
          {
            const std::string& sub_c(_options.no_substitution ? c : normalize_character(c, v));
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

            if (_options.mode == Mode::Conservative)
            {
              if (is_number
                  || (sub_c[0] == '-' && letter)
                  || (sub_c[0] == '_')
                  || (letter && (sub_c[0] == '.' || sub_c[0] == ',') && (unicode::is_number(next_v) || unicode::is_letter(next_v))))
                {
                  is_letter = true;
                  alphabet = number_alphabet;
                }
            }

            if (is_letter && _options.mode != Mode::Char)
            {
              const unicode::CaseType case_type = unicode::get_case_v2(v);
              const Casing new_casing = update_casing(token.casing,
                                                      case_type,
                                                      token.unicode_length());

              bool segment_case = false;
              bool segment_alphabet = false;
              bool segment_alphabet_change = false;
              if ((!letter && !space)
                  || (letter &&
                      ((segment_alphabet = (prev_alphabet == alphabet
                                            && alphabet >= 0
                                            && (_options.segment_alphabet_codes.find(alphabet)
                                                != _options.segment_alphabet_codes.end())))
                       || (segment_alphabet_change = (prev_alphabet != alphabet
                                                      && _options.segment_alphabet_change))
                       || (prev_alphabet == placeholder_alphabet)
                       || (_options.segment_case
                           && (segment_case = (new_casing == Casing::Mixed))))))
              {
                token.join_right = true;
                if (_options.preserve_segmented_tokens
                    && (segment_case || segment_alphabet || segment_alphabet_change))
                  token.preserve = true;
                annotated_tokens.emplace_back(std::move(token));
                token = Token();
                token.casing = update_casing(token.casing, case_type, 0);
              }
              else
              {
                token.casing = new_casing;
                if (other && token.empty())
                  annotated_tokens.back().join_right = true;
              }

              token.append(sub_c);
              state = State::Letter;
              prev_alphabet = alphabet;
            }
            else if (is_number && _options.mode != Mode::Char)
            {
              if (letter || (number && _options.segment_numbers) || (!number && !space))
              {
                Token next_token;
                if (!letter || prev_alphabet == placeholder_alphabet)
                  token.join_right = true;
                else
                  next_token.join_left = true;
                if (_options.preserve_segmented_tokens && number && _options.segment_numbers)
                  token.preserve = true;
                annotated_tokens.emplace_back(std::move(token));
                std::swap(token, next_token);
              }
              else if (other)
              {
                annotated_tokens.back().join_right = true;
              }

              token.append(sub_c);
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

              if (sub_c[0] == ' ' && !_options.no_substitution)
                token.append(protected_character + int_to_hex(sub_c[0]) + sub_c.substr(1));
              else
                token.append(sub_c);

              annotated_tokens.emplace_back(std::move(token));
              token = Token();
              state = State::Other | State::Space;
            }
          }
        }
      }

      if (!token.empty())
        annotated_tokens.emplace_back(std::move(token));
    }
  }

  static inline void add_final_token(std::vector<std::string>& tokens,
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
    if (_options.case_feature)
      num_features += 1;

    for (size_t i = 0; i < num_features; ++i)
    {
      features.emplace_back(0);
      features.back().reserve(annotated_tokens.size());
    }

    std::vector<TokenCaseMarkup> case_markups;
    if (_options.case_markup)
      case_markups = get_case_markups(annotated_tokens, _options.soft_case_regions);

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

      if (_options.case_markup && case_markups[i].prefix != CaseMarkupType::None)
        tokens.emplace_back(write_case_markup(case_markups[i].prefix, case_markups[i].casing));

      const std::string* prefix = nullptr;
      const std::string* suffix = nullptr;
      bool attach = !token.preserve;

      if (_options.joiner_annotate)
      {
        if (token.join_left && i > 0)
          prefix = &_options.joiner;
        if (token.join_right && i + 1 < annotated_tokens.size())
          suffix = &_options.joiner;
        if (token.spacer)
          attach = true;  // Ignore preserve flag for spacers in joiner mode.
        attach = attach && !_options.joiner_new;
      }
      else if (_options.spacer_annotate)
      {
        if ((i == 0 && token.spacer)
            || (i > 0 && !token.join_left && !annotated_tokens[i - 1].join_right))
          prefix = &spacer_marker;
        attach = attach && !_options.spacer_new;
      }

      if (!prefix && !suffix)
        add_final_token(tokens, features, _options.case_feature, str, casing);
      else if (attach)
      {
        std::string final_token = (prefix ? *prefix : "") + str + (suffix ? *suffix : "");
        add_final_token(tokens, features, _options.case_feature, std::move(final_token), casing);
      }
      else
      {
        if (prefix)
          add_final_token(tokens, features, _options.case_feature, *prefix);
        add_final_token(tokens, features, _options.case_feature, str, casing);
        if (suffix)
          add_final_token(tokens, features, _options.case_feature, *suffix);
      }

      if (_options.case_markup && case_markups[i].suffix != CaseMarkupType::None)
        tokens.emplace_back(write_case_markup(case_markups[i].suffix, case_markups[i].casing));
    }
  }

  Tokenizer& Tokenizer::set_joiner(const std::string& joiner)
  {
    _options.joiner = joiner;
    return *this;
  }

  void Tokenizer::unset_annotate() {
    _options.joiner_annotate = _options.spacer_annotate = false;
  }

  void Tokenizer::set_subword_encoder(const std::shared_ptr<const SubwordEncoder>& subword_encoder)
  {
    _subword_encoder = subword_encoder;
    if (_subword_encoder)
      _subword_encoder->update_tokenization_options(_options);
  }

  bool Tokenizer::add_alphabet_to_segment(const std::string& alphabet)
  {
    return _options.add_alphabet_to_segment(alphabet);
  }

  bool Tokenizer::is_placeholder(const std::string& str)
  {
    return ::onmt::is_placeholder(str);
  }

}
