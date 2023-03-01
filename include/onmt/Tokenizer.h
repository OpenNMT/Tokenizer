#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/ITokenizer.h"
#include "onmt/Token.h"

namespace onmt
{

  void OPENNMTTOKENIZER_EXPORT set_random_seed(const unsigned int seed);

  class SubwordEncoder;

  class OPENNMTTOKENIZER_EXPORT Tokenizer: public ITokenizer
  {
  public:
    enum class Mode
    {
      Conservative,
      Aggressive,
      Char,
      Space,
      None
    };

    static Mode str_to_mode(const std::string& mode);
    static std::string mode_to_str(const Mode mode);

    // See https://github.com/OpenNMT/Tokenizer/blob/master/docs/options.md for more details.
    struct Options
    {
      Mode mode = Mode::Conservative;
      std::string lang;
      bool no_substitution = false;
      bool case_feature = false;
      bool case_markup = false;
      bool soft_case_regions = false;
      bool with_separators = false;
      bool allow_isolated_marks = false;
      bool joiner_annotate = false;
      bool joiner_new = false;
      std::string joiner;
      bool spacer_annotate = false;
      bool spacer_new = false;
      bool preserve_placeholders = false;
      bool preserve_segmented_tokens = false;
      bool support_prior_joiners = false;
      bool segment_case = false;
      bool segment_numbers = false;
      bool segment_alphabet_change = false;
      std::vector<std::string> segment_alphabet;

      Options() = default;
      Options(Mode mode, int legacy_flags, const std::string& joiner = joiner_marker);

      void validate();

    private:
      bool add_alphabet_to_segment(const std::string& alphabet);
      std::unordered_set<int> segment_alphabet_codes;

      friend class Tokenizer;
    };

    static const std::string joiner_marker;
    static const std::string spacer_marker;
    static const std::string ph_marker_open;
    static const std::string ph_marker_close;
    static const std::string escaped_character_prefix;
    static const size_t escaped_character_width;

    Tokenizer(Options options,
              const std::shared_ptr<const SubwordEncoder>& subword_encoder = nullptr);

    using ITokenizer::tokenize;
    using ITokenizer::detokenize;

    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features,
                  bool training = true) const override;
    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features,
                  std::unordered_map<std::string, size_t>& alphabets,
                  bool training = true) const override;
    void tokenize(const std::string& text,
                  std::vector<Token>& annotated_tokens,
                  bool training = true) const;

    Token annotate_token(const std::string& word) const;
    void annotate_tokens(const std::vector<std::string>& words,
                         const std::vector<std::vector<std::string>>& features,
                         std::vector<Token>& tokens) const;
    void finalize_tokens(const std::vector<Token>& annotated_tokens,
                         std::vector<std::string>& tokens,
                         std::vector<std::vector<std::string>>& features) const;
    std::string detokenize(const std::vector<Token>& tokens) const;
    std::string detokenize(const std::vector<Token>& tokens,
                           Ranges& ranges, bool merge_ranges = false) const;

    std::string detokenize(const std::vector<std::string>& words,
                           const std::vector<std::vector<std::string> >& features) const override;
    std::string detokenize(const std::vector<std::string>& words,
                           const std::vector<std::vector<std::string> >& features,
                           Ranges& ranges, bool merge_ranges = false) const override;

    void set_subword_encoder(const std::shared_ptr<const SubwordEncoder>& subword_encoder);

    const std::shared_ptr<const SubwordEncoder>& get_subword_encoder() const
    {
      return _subword_encoder;
    }

    const Options& get_options() const
    {
      return _options;
    }

  private:
    Options _options;
    std::shared_ptr<const SubwordEncoder> _subword_encoder;

    void tokenize_on_placeholders(const std::string& text,
                                  std::vector<Token>& annotated_tokens) const;
    void tokenize_text(const std::string& text,
                       std::vector<Token>& annotated_tokens,
                       std::unordered_map<std::string, size_t>* alphabets) const;

    void tokenize(const std::string& text,
                  std::vector<Token>& annotated_tokens,
                  std::unordered_map<std::string, size_t>* alphabets,
                  bool training) const;
    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features,
                  std::unordered_map<std::string, size_t>* alphabets,
                  bool training) const;
    std::string detokenize(const std::vector<Token>& tokens,
                           Ranges* ranges,
                           bool merge_ranges = false,
                           const std::vector<size_t>* index_map = nullptr) const;
    std::string detokenize(const std::vector<std::string>& words,
                           const std::vector<std::vector<std::string> >& features,
                           Ranges* ranges, bool merge_ranges = false) const;

    void parse_tokens(const std::vector<std::string>& words,
                      const std::vector<std::vector<std::string>>& features,
                      std::vector<Token>& tokens,
                      std::vector<size_t>* index_map = nullptr) const;

  public:
    // The symbols below are deprecated but kept for backward compatibility.
    enum Flags
    {
      None = 0,
      CaseFeature = 1 << 0,
      JoinerAnnotate = 1 << 1,
      JoinerNew = 1 << 2,
      WithSeparators = 1 << 3,
      SegmentCase = 1 << 4,
      SegmentNumbers = 1 << 5,
      SegmentAlphabetChange = 1 << 6,
      CacheBPEModel = 1 << 7,  // Deprecated.
      NoSubstitution = 1 << 8,  // Do not replace special characters.
      SpacerAnnotate = 1 << 9,
      CacheModel = 1 << 10,  // Deprecated.
      SentencePieceModel = 1 << 11,
      PreservePlaceholders = 1 << 12,
      SpacerNew = 1 << 13,
      PreserveSegmentedTokens = 1 << 14,
      CaseMarkup = 1 << 15,
      SupportPriorJoiners = 1 << 16,
      SoftCaseRegions = 1 << 17,
    };

    Tokenizer(Mode mode,
              int flags = Flags::None,
              const std::string& model_path = "",
              const std::string& joiner = joiner_marker,
              const std::string& vocab_path = "",
              int vocab_threshold = 50);

    // External subword encoder constructor.
    // Note: the tokenizer takes ownership of the subword_encoder pointer.
    Tokenizer(Mode mode,
              const SubwordEncoder* subword_encoder,
              int flags = Flags::None,
              const std::string& joiner = joiner_marker);

    // SentencePiece-specific constructor.
    Tokenizer(const std::string& sp_model_path,
              int sp_nbest_size = 0,
              float sp_alpha = 0.1,
              Mode mode = Mode::None,
              int flags = Flags::None,
              const std::string& joiner = joiner_marker);

    Tokenizer& set_joiner(const std::string& joiner);
    void unset_annotate();
    bool add_alphabet_to_segment(const std::string& alphabet);
    static bool is_placeholder(const std::string& str);

  };

}
