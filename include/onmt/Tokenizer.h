#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/ITokenizer.h"
#include "onmt/SubwordEncoder.h"

namespace onmt
{

  void set_random_seed(const unsigned int seed);

  // This Tokenizer implements the behaviour of OpenNMT's tools/tokenize.lua.
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

    static const std::string joiner_marker;
    static const std::string spacer_marker;
    static const std::string ph_marker_open;
    static const std::string ph_marker_close;

    Tokenizer(Mode mode,
              int flags = Flags::None,
              const std::string& model_path = "",
              const std::string& joiner = joiner_marker,
              const std::string& vocab_path = "",
              int vocab_threshold = 50);

    // External subword encoder constructor.
    // Note: the tokenizer takes ownership of the subword_encoder pointer.
    Tokenizer(Mode mode,
              SubwordEncoder* subword_encoder,
              int flags = Flags::None,
              const std::string& joiner = joiner_marker);

    // SentencePiece-specific constructor.
    Tokenizer(const std::string& sp_model_path,
              int sp_nbest_size = 0,
              float sp_alpha = 0.1,
              Mode mode = Mode::None,
              int flags = Flags::None,
              const std::string& joiner = joiner_marker);

    using ITokenizer::tokenize;
    using ITokenizer::detokenize;

    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features) const override;

    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features,
                  std::unordered_map<std::string, size_t>& alphabets) const override;

    void tokenize(const std::string& text,
                  std::vector<Token>& annotated_tokens) const;
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

    Tokenizer& set_joiner(const std::string& joiner);

    void set_subword_encoder(const std::shared_ptr<SubwordEncoder>& subword_encoder);

    void unset_annotate();

    bool add_alphabet_to_segment(const std::string& alphabet);
    bool is_alphabet_to_segment(const std::string& alphabet) const;
    bool is_alphabet_to_segment(int alphabet) const;

    static bool is_placeholder(const std::string& str);

  private:
    static const int placeholder_alphabet = -2;
    static const int number_alphabet = -3;

    Mode _mode;

    bool _case_feature;
    bool _case_markup;
    bool _soft_case_regions;
    bool _joiner_annotate;
    bool _joiner_new;
    bool _with_separators;
    bool _segment_case;
    bool _segment_numbers;
    bool _segment_alphabet_change;
    bool _no_substitution;
    bool _spacer_annotate;
    bool _spacer_new;
    bool _preserve_placeholders;
    bool _preserve_segmented_tokens;
    bool _support_prior_joiners;

    std::shared_ptr<SubwordEncoder> _subword_encoder;
    std::string _joiner;

    std::unordered_set<int> _segment_alphabet;

    void read_flags(int flags);

    void tokenize_on_placeholders(const std::string& text,
                                  std::vector<Token>& annotated_tokens) const;
    void tokenize_on_spaces(const std::string& text,
                            std::vector<Token>& annotated_tokens) const;
    void tokenize_text(const std::string& text,
                       std::vector<Token>& annotated_tokens,
                       std::unordered_map<std::string, size_t>* alphabets) const;

    void tokenize(const std::string& text,
                  std::vector<Token>& annotated_tokens,
                  std::unordered_map<std::string, size_t>* alphabets) const;
    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features,
                  std::unordered_map<std::string, size_t>* alphabets) const;
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
  };

}
