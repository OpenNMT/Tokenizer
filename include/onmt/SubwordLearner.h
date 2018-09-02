#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace onmt
{
  class Tokenizer;

  class SubwordLearner
  {
  public:
    SubwordLearner(bool verbose, Tokenizer *pTok=0);
    virtual ~SubwordLearner() = default;
    virtual void ingest(std::istream &) = 0;
    virtual void learn(std::ostream &) = 0;
  protected:
    bool _verbose;
    Tokenizer *_pTok;
  };

}
