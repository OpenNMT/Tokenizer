#pragma once

#include <unordered_map>
#include <set>

#include "onmt/ITokenizer.h"
#include "onmt/BPE.h"

namespace onmt
{

  // This Tokenizer implements the behaviour of OpenNMT's tools/tokenize.lua.
  class Tokenizer: public ITokenizer
  {
  public:
    enum class Mode
    {
      Conservative,
      Aggressive,
      Space
    };

    enum Flags
    {
      None = 0,
      CaseFeature = 1,
      JoinerAnnotate = 2,
      JoinerNew = 4,
      WithSeparators = 8,
      SegmentCase = 16,
      SegmentNumbers = 32,
      SegmentAlphabetChange = 64,
      CacheBPEModel = 128,
      NoSubstitution = 256,  // Do not replace special characters.
      SpacerAnnotate = 512
    };

    static const std::string joiner_marker;
    static const std::string spacer_marker;
    static const std::string ph_marker_open;
    static const std::string ph_marker_close;

    static const std::unordered_map<std::string, onmt::Tokenizer::Mode> mapMode;

    Tokenizer(Mode mode,
              int flags = Flags::None,
              const std::string& bpe_model_path = "",
              const std::string& joiner = joiner_marker);
    ~Tokenizer();

    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features) const override;

    std::string detokenize(const std::vector<std::string>& words,
                           const std::vector<std::vector<std::string> >& features) const override;

    Tokenizer& set_joiner(const std::string& joiner);
    Tokenizer& set_bpe_model(const std::string& model_path, bool cache_model = false);
    bool add_alphabet_to_segment(const std::string& alphabet);
    bool is_alphabet_to_segment(const std::string& alphabet) const;

  private:
    Mode _mode;

    bool _case_feature;
    bool _joiner_annotate;
    bool _joiner_new;
    bool _with_separators;
    bool _segment_case;
    bool _segment_numbers;
    bool _segment_alphabet_change;
    bool _cache_bpe_model;
    bool _no_substitution;
    bool _spacer_annotate;

    const BPE* _bpe;
    std::string _joiner;

    std::set<std::string> _segment_alphabet;

    std::vector<std::string> bpe_segment(const std::vector<std::string>& tokens) const;

    bool has_left_join(const std::string& word) const;
    bool has_right_join(const std::string& word) const;
  };

}
