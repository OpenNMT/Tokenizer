#include "onmt/Tokenizer.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>

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

  static const std::string& normalize_character(const std::string& c) {
    auto it = std::find(special_chars.begin(), special_chars.end(), c);
    if (it != special_chars.end())
      return substitutes[std::distance(special_chars.begin(), it)];
    return c;
  }

  static void annotate_case(std::vector<AnnotatedToken>& annotated_tokens)
  {
    for (auto& token : annotated_tokens)
    {
      if (Tokenizer::is_placeholder(token.str()))
        continue;
      auto pair = CaseModifier::extract_case_type(token.str());
      token.set(std::move(pair.first));
      token.set_case(pair.second);
      if (pair.second == CaseModifier::Type::Uppercase)
      {
        token.set_case_region_begin(pair.second);
        token.set_case_region_end(pair.second);
      }
    }
  }

  // Returns true if the token at offset can be connected to an uppercase token or
  // capital letter. This useful to avoid closing an uppercase region if intermediate
  // tokens are case invariant.
  static bool has_connected_uppercase(std::vector<AnnotatedToken>& tokens, size_t offset)
  {
    for (size_t j = offset + 1; j < tokens.size(); ++j)
    {
      auto case_type = tokens[j].get_case();
      if (case_type == CaseModifier::Type::Uppercase
          || case_type == CaseModifier::Type::CapitalizedFirst)
        return true;
      else if (case_type != CaseModifier::Type::None)
        break;
    }
    return false;
  }

  // Returns true if token only contains numbers.
  static bool numbers_only(const std::string& token)
  {
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points;
    unicode::explode_utf8(token, chars, code_points);
    return std::all_of(code_points.begin(), code_points.end(), unicode::is_number);
  }

  // Define uppercase regions in a sequence of tokens.
  // This function tries to minimize the number of regions by possibly including case
  // invariant characters (numbers, symbols, etc.) in uppercase regions.
  static void set_soft_case_regions(std::vector<AnnotatedToken>& tokens)
  {
    // Reset previous annotations.
    for (auto& token : tokens)
    {
      token.set_case_region_begin(CaseModifier::Type::None);
      token.set_case_region_end(CaseModifier::Type::None);
    }

    bool in_uppercase_region = false;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      auto case_type = tokens[i].get_case();

      if (in_uppercase_region)
      {
        // End region on lowercase tokens or case invariant tokens that are not numbers
        // or not followed by another uppercase sequence.
        if (case_type == CaseModifier::Type::Uppercase
            || case_type == CaseModifier::Type::CapitalizedFirst
            || (case_type == CaseModifier::Type::None
                && !Tokenizer::is_placeholder(tokens[i].str())
                && (has_connected_uppercase(tokens, i) || numbers_only(tokens[i].str()))))
        {
          // Mark intermediate token as uppercase.
          tokens[i].set_case(CaseModifier::Type::Uppercase);
        }
        else
        {
          tokens[i - 1].set_case_region_end(CaseModifier::Type::Uppercase);
          in_uppercase_region = false;
        }
      }
      else
      {
        // Begin region on uppercase tokens or capital letter that are followed by
        // another uppercase sequence.
        if (case_type == CaseModifier::Type::Uppercase
            || (case_type == CaseModifier::Type::CapitalizedFirst
                && has_connected_uppercase(tokens, i)))
        {
          tokens[i].set_case_region_begin(CaseModifier::Type::Uppercase);
          in_uppercase_region = true;
        }
      }
    }

    // Close last region, if any.
    if (in_uppercase_region)
      tokens.back().set_case_region_end(CaseModifier::Type::Uppercase);
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
#ifdef WITH_SP
    if (dynamic_cast<const SentencePiece*>(subword_encoder) != nullptr
        && _mode == Mode::None && !_joiner_annotate && !_spacer_annotate)
    {
      _spacer_annotate = true;
      _no_substitution = true;
    }
#endif
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
    _soft_case_regions = flags & Flags::SoftCaseRegions;
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
    _support_prior_joiners = flags & Flags::SupportPriorJoiners;

    if (_case_feature && _case_markup)
      throw std::invalid_argument("case_feature and case_markup can't be set at the same time");
    if (_joiner_annotate && _spacer_annotate)
      throw std::invalid_argument("joiner_annotate and spacer_annotate can't be set at the same time");
    if (_spacer_new && !_spacer_annotate)
      throw std::invalid_argument("spacer_new requires spacer_annotate");
    if (_joiner_new && !_joiner_annotate)
      throw std::invalid_argument("joiner_new requires joiner_annotate");
  }

  Tokenizer::~Tokenizer()
  {
    if (!_cache_model)
      delete _subword_encoder;
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

  std::string Tokenizer::detokenize(const std::vector<AnnotatedToken>& tokens) const
  {
    return detokenize(tokens, nullptr);
  }

  std::string Tokenizer::detokenize(const std::vector<AnnotatedToken>& tokens,
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

  std::string Tokenizer::detokenize(const std::vector<AnnotatedToken>& tokens,
                                    Ranges* ranges, bool merge_ranges) const
  {
    std::string line;
    line.reserve(tokens.size() * 10);

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      const auto& token = tokens[i];
      if (i > 0 && !tokens[i - 1].is_joined_right() && !token.is_joined_left())
        line += ' ';

      std::string prep_word = token.str();

      if (!is_placeholder(prep_word))
      {
        auto case_modifier = token.get_case();
        if (case_modifier != CaseModifier::Type::None)
          prep_word = CaseModifier::apply_case(prep_word, case_modifier);

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
                          std::forward_as_tuple(token.get_index()),
                          std::forward_as_tuple(line.size(), line.size() + prep_word.size() - 1));

        line.append(prep_word);
      }
    }

    if (ranges && merge_ranges)
      *ranges = merge_consecutive_ranges(line, *ranges);

    return line;
  }

  void Tokenizer::annotate_tokens(const std::vector<std::string>& words,
                                  const std::vector<std::vector<std::string>>& features,
                                  std::vector<AnnotatedToken>& tokens) const
  {
    tokens.reserve(words.size());
    CaseModifier::Type case_region = CaseModifier::Type::None;
    CaseModifier::Type case_modifier = CaseModifier::Type::None;

    for (size_t i = 0; i < words.size(); ++i)
    {
      size_t features_offset = 0;
      if (_case_feature)
      {
        if (features.empty())
          throw std::runtime_error("Missing case feature");
        case_modifier = CaseModifier::char_to_type(features[0][i][0]);
        features_offset = 1;
      }
      else
      {
        auto case_markup = CaseModifier::get_case_markup(words[i]);
        switch (case_markup)
        {
        case CaseModifier::Markup::RegionBegin:
          case_region = CaseModifier::get_case_modifier_from_markup(words[i]);
          case_modifier = CaseModifier::Type::None;
          continue;
        case CaseModifier::Markup::RegionEnd:
          case_region = CaseModifier::Type::None;
          case_modifier = CaseModifier::Type::None;
          continue;
        case CaseModifier::Markup::Modifier:
          case_modifier = CaseModifier::get_case_modifier_from_markup(words[i]);
          continue;
        default:
          case_modifier = (case_modifier != CaseModifier::Type::None ? case_modifier : case_region);
          break;
        }
      }

      const std::string& word = words[i];
      size_t subpos = 0;
      size_t sublen = word.size();
      AnnotatedToken token;

      if (_spacer_annotate)
      {
        if (has_left_marker(word, spacer_marker))
        {
          subpos += spacer_marker.length();
          sublen -= spacer_marker.length();
        }
        else
          token.join_left();
      }
      else
      {
        if (has_right_join(word))
        {
          token.join_right();
          sublen -= _joiner.length();
        }
        if (has_left_join(word))
        {
          token.join_left();
          subpos += _joiner.length();
          sublen -= _joiner.length();
        }
      }

      token.set(word.substr(subpos, sublen));
      token.set_case(case_modifier);
      token.set_index(i);
      if (!features.empty())
      {
        for (size_t j = features_offset; j < features.size(); ++j)
          token.insert_feature(features[j][i]);
      }
      // Forward the case modifier if the current token is a joiner or spacer.
      if (!token.str().empty())
        case_modifier = CaseModifier::Type::None;

      tokens.emplace_back(std::move(token));
    }
  }

  std::string Tokenizer::detokenize(const std::vector<std::string>& words,
                                    const std::vector<std::vector<std::string> >& features,
                                    Ranges* ranges, bool merge_ranges) const
  {
    std::vector<AnnotatedToken> tokens;
    annotate_tokens(words, features, tokens);
    return detokenize(tokens, ranges, merge_ranges);
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
                           std::vector<AnnotatedToken>& annotated_tokens) const {
    return tokenize(text, annotated_tokens, nullptr);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<std::string>& words,
                           std::vector<std::vector<std::string> >& features,
                           std::unordered_map<std::string, size_t>* alphabets) const
  {
    std::vector<AnnotatedToken> annotated_tokens;
    tokenize(text, annotated_tokens, alphabets);
    finalize_tokens(annotated_tokens, words, features);
  }

  void Tokenizer::tokenize(const std::string& text,
                           std::vector<AnnotatedToken>& annotated_tokens,
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
      annotated_tokens = encode_subword(annotated_tokens);
    if (_soft_case_regions)
      set_soft_case_regions(annotated_tokens);
  }

  void Tokenizer::tokenize_on_placeholders(const std::string& text,
                                           std::vector<AnnotatedToken>& tokens) const
  {
    // Split on characters.
    std::vector<std::string> chars;
    std::vector<unicode::code_point_t> code_points_main;
    std::vector<std::vector<unicode::code_point_t>> code_points_combining;
    unicode::explode_utf8_with_marks(text, chars, code_points_main, code_points_combining);

    AnnotatedToken token;  // Accumulate characters in this.
    bool in_placeholder = false;

    for (size_t i = 0; i < chars.size(); ++i)
    {
      const auto& c = chars[i];

      if (!in_placeholder)
      {
        if (_support_prior_joiners && c == _joiner)
        {
          // Mark joint but discard character.
          if (token.str().empty())
            token.join_left();
          else
            token.join_right();
        }
        else if (c == ph_marker_open)
        {
          if (!token.str().empty())
          {
            // Flush accumulated token and mark joint if it did not finish by a separator.
            if (i > 0 && !unicode::is_separator(code_points_main[i - 1]))
              token.join_right();
            if (_preserve_segmented_tokens)
              token.preserve();
            tokens.emplace_back(std::move(token));
            token.clear();
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
            token.join_right();
          if (_preserve_placeholders || _preserve_segmented_tokens)
            token.preserve();
          tokens.emplace_back(std::move(token));
          token.clear();
          in_placeholder = false;
        }
      }
    }

    // Flush remaining token.
    if (!token.str().empty())
      tokens.emplace_back(std::move(token));
  }

  void Tokenizer::tokenize_on_spaces(const std::string& text,
                                     std::vector<AnnotatedToken>& annotated_tokens) const
  {
    {
      std::vector<std::string> chunks = unicode::split_utf8(text, " ");
      for (auto& chunk: chunks)
      {
        if (chunk.empty())
          continue;

        std::vector<std::string> fields = unicode::split_utf8(chunk, ITokenizer::feature_marker);
        auto& token = fields[0];

        std::vector<AnnotatedToken> sub_tokens;
        tokenize_on_placeholders(token, sub_tokens);

        for (size_t i = 1; i < fields.size(); ++i)
        {
          // Replicate the features to each sub token.
          for (auto& sub_token : sub_tokens)
            sub_token.insert_feature(fields[i]);
        }

        annotated_tokens.insert(annotated_tokens.end(), sub_tokens.begin(), sub_tokens.end());
      }
    }
  }

  void Tokenizer::tokenize_text(const std::string& text,
                                std::vector<AnnotatedToken>& annotated_tokens,
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

      AnnotatedToken token;

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
            annotated_tokens.back().join_right();
            continue;
          }
          else if (space) {
            token.join_left();
            continue;
          } else {
            token.join_right();
            annotated_tokens.emplace_back(std::move(token));
            token.clear();
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
              token.preserve();
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
        else if (is_separator)
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
          // skip special characters and BOM
          if (v >= 32 && v != 0xFEFF)
          {
            const std::string& sub_c(_no_substitution ? c : normalize_character(c));
            bool cur_letter = unicode::is_letter(v);
            bool cur_number = !cur_letter && unicode::is_number(v);

            int alphabet = is_alphabet(v, prev_alphabet) ? prev_alphabet : get_alphabet_id(v);
            if (alphabets != nullptr)
            {
              if (alphabet >= 0 && cur_letter)
                (*alphabets)[id_to_alphabet(static_cast<Alphabet>(alphabet))]++;
              else
                (*alphabets)[cur_number ? "Numeric" : "Other"]++;
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
                  || (letter &&
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

              if (sub_c[0] == ' ' && !_no_substitution)
                token.append(protected_character + "0020" + sub_c.substr(1));
              else
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
  }

  template <typename T>
  void add_final_token(std::vector<std::string>& tokens,
                       std::vector<std::vector<std::string>>& features,
                       bool case_feature,
                       T&& token,
                       CaseModifier::Type case_type = CaseModifier::Type::None)
  {
    if (token.empty())
      return;
    tokens.emplace_back(std::forward<T>(token));  // Forward lvalue or rvalue as is.
    if (case_feature)
      features.back().emplace_back(1, CaseModifier::type_to_char(case_type));
  }

  void Tokenizer::finalize_tokens(std::vector<AnnotatedToken>& annotated_tokens,
                                  std::vector<std::string>& tokens,
                                  std::vector<std::vector<std::string>>& features) const
  {
    tokens.reserve(annotated_tokens.size());
    size_t num_features = 0;
    if (annotated_tokens.size() > 0 && annotated_tokens[0].has_features())
      num_features = annotated_tokens[0].features().size();
    if (_case_feature)
      num_features += 1;

    for (size_t i = 0; i < num_features; ++i)
    {
      features.emplace_back(0);
      features.back().reserve(annotated_tokens.size());
    }

    for (size_t i = 0; i < annotated_tokens.size(); ++i)
    {
      auto& token = annotated_tokens[i];
      auto& str = token.get_str();  // Non const getter to allow moving str into tokens.
      const auto case_type = token.get_case();

      if (token.has_features())
      {
        const auto& token_features = token.features();
        for (size_t j = 0; j < token_features.size(); ++j)
          features[j].push_back(token_features[j]);
      }

      if (_case_markup)
      {
        if (token.begin_case_region())
          tokens.emplace_back(CaseModifier::generate_case_markup_begin(token.get_case_region_begin()));
        else if (case_type == CaseModifier::Type::Capitalized
                 || case_type == CaseModifier::Type::CapitalizedFirst)
          tokens.emplace_back(CaseModifier::generate_case_markup(case_type));
      }

      const std::string* prefix = nullptr;
      const std::string* suffix = nullptr;
      bool attach = !token.should_preserve();

      if (_joiner_annotate)
      {
        if (token.is_joined_left() && i > 0)
          prefix = &_joiner;
        if (token.is_joined_right() && i + 1 < annotated_tokens.size())
          suffix = &_joiner;
        attach = attach && !_joiner_new;
      }
      else if (_spacer_annotate)
      {
        bool joined_left = (token.is_joined_left()
                            || (i > 0 && annotated_tokens[i - 1].is_joined_right()));
        if (!joined_left && (i != 0 || token.is_spacer()))
          prefix = &spacer_marker;
        attach = attach && !_spacer_new;
      }

      if (!prefix && !suffix)
        add_final_token(tokens, features, _case_feature, std::move(str), case_type);
      else if (attach)
      {
        std::string final_token = (prefix ? *prefix : "") + str + (suffix ? *suffix : "");
        add_final_token(tokens, features, _case_feature, std::move(final_token), case_type);
      }
      else
      {
        if (prefix)
          add_final_token(tokens, features, _case_feature, *prefix);
        add_final_token(tokens, features, _case_feature, std::move(str), case_type);
        if (suffix)
          add_final_token(tokens, features, _case_feature, *suffix);
      }

      if (_case_markup && token.end_case_region())
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
    {
      _spacer_annotate = true;
      _no_substitution = true;
    }
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
