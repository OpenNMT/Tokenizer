#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordEncoder.h"
#include "onmt/unicode/Unicode.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT BPE: public SubwordEncoder
  {
  public:
    BPE(const std::string& model_path, const float dropout = 0);
    BPE(const std::string& model_path, const std::string& joiner, const float dropout = 0);

    std::vector<std::string> encode(const std::string& str, bool training = true) const override;
    std::vector<Token> encode_and_annotate(const Token& token, bool training = true) const override;

    void set_vocabulary(const std::vector<std::string>& vocabulary,
                        const Tokenizer::Options* options = nullptr) override;
    void reset_vocabulary() override;

    void set_joiner(const std::string& joiner)
    {
      _tokenization_options.joiner = joiner;
    }

    void set_dropout(const float dropout)
    {
      _dropout = dropout;
    }

    static std::vector<std::string> get_initial_pieces(const std::vector<unicode::CharInfo>& chars,
                                                       const bool lowercase = false);

  private:
    std::string _end_of_word;
    std::string _begin_of_word;
    bool _prefix;
    bool _suffix;
    bool _case_insensitive;
    std::pair<int, int> _version;
    float _dropout;

    // Tokenization options used to produce the vocabulary passed to set_vocabulary.
    Tokenizer::Options _tokenization_options;

    std::unordered_map<std::string, int> _codes;
    std::unordered_map<std::string, std::pair<std::string, std::string> > _codes_reverse;
    std::unordered_set<std::string> _bpe_vocab;

    void load_model(const std::string& model_path);

    int get_score(const std::string& gram1, const std::string& gram2) const;
    void apply_merges(std::vector<std::string>& chars, bool training) const;

    bool in_vocabulary(const std::string& token) const;
    bool in_vocabulary(const onmt::Token& token, const bool first, const bool last) const;
    std::vector<Token> check_vocab_and_split(std::vector<Token> pieces) const;
    void recursive_split(Token piece,
                         std::vector<Token>& pieces_in_vocab,
                         const bool first,
                         const bool last) const;
  };

}
