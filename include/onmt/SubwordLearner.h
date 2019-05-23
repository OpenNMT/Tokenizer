#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "onmt/opennmttokenizer_export.h"

namespace onmt
{
  class Tokenizer;

  class OPENNMTTOKENIZER_EXPORT SubwordLearner
  {
  public:
    SubwordLearner(bool verbose);
    virtual ~SubwordLearner() = default;
    virtual void ingest(std::istream& in, const Tokenizer* tokenizer = nullptr) = 0;
    virtual void learn(std::ostream& out, const char* description = nullptr) = 0;
  protected:
    bool _verbose;
    std::unique_ptr<const Tokenizer> _default_tokenizer;
  };

}
