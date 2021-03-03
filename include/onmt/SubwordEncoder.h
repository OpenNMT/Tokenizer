#pragma once

#include <string>
#include <vector>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/Tokenizer.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT SubwordEncoder
  {
  public:
    virtual ~SubwordEncoder() = default;

    // Maybe update the tokenization options for this subword encoder.
    virtual void update_tokenization_options(Tokenizer::Options& options) const;

    virtual void load_vocabulary(const std::string& path,
                                 int frequency_threshold,
                                 const Tokenizer::Options* tokenization_options = nullptr);
    virtual void set_vocabulary(const std::vector<std::string>& vocabulary,
                                const Tokenizer::Options* tokenization_options = nullptr);
    virtual void reset_vocabulary();

    virtual std::vector<std::string> encode(const std::string& str,
                                            bool training = true) const = 0;
    virtual std::vector<Token> encode_and_annotate(const Token& token,
                                                   bool training = true) const = 0;
    virtual std::vector<Token> encode_and_annotate(const std::vector<Token>& tokens,
                                                   bool training = true) const;

    static void propagate_token_properties(const Token& token, std::vector<Token>& tokens);
  };

}
