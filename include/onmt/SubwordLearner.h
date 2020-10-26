#pragma once

#include <memory>
#include <string>
#include <iostream>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/Tokenizer.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT SubwordLearner
  {
  public:
    SubwordLearner(bool verbose, const Tokenizer* default_tokenizer = nullptr);
    virtual ~SubwordLearner() = default;
    virtual void ingest_token(const Token& token);
    virtual void ingest_token(const std::string& token, const Tokenizer* tokenizer = nullptr);
    virtual void ingest(const std::string& text, const Tokenizer* tokenizer = nullptr);
    virtual void ingest(std::istream& in, const Tokenizer* tokenizer = nullptr);
    virtual void learn(std::ostream& out, const char* description = nullptr, bool verbose = false) = 0;
    virtual void learn(const std::string& model_path,
                       const char* description = nullptr,
                       bool verbose = false);

    const std::shared_ptr<const Tokenizer>& get_default_tokenizer() const;
  protected:
    virtual void ingest_token_impl(const std::string& token) = 0;
    bool _verbose;
    std::shared_ptr<const Tokenizer> _default_tokenizer;
  };

}
