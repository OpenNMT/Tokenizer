#include "onmt/SubwordLearner.h"

#include <sstream>

namespace onmt
{

  SubwordLearner::SubwordLearner(bool verbose)
    : _verbose(verbose)
  {
  }

  void SubwordLearner::ingest(const std::string& text, const Tokenizer* tokenizer)
  {
    std::istringstream in(text);
    ingest(in, tokenizer);
  }

}
