#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "opennmttokenizer_export.h"

namespace onmt
{
  class Tokenizer;

  class OPENNMTTOKENIZER_EXPORT SubwordLearner
  {
  public:
    SubwordLearner(bool verbose);
    virtual ~SubwordLearner();
    virtual void ingest(std::istream &, Tokenizer *) = 0;
    virtual void learn(std::ostream &, const char *) = 0;
  protected:
    bool _verbose;
    Tokenizer *_pTokDefault;
  };

}
