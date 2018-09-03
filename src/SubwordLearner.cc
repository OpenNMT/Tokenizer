#include "onmt/SubwordLearner.h"
#include <onmt/Tokenizer.h>

namespace onmt
{

  SubwordLearner::SubwordLearner(bool verbose):
        _verbose(verbose) {
    _pTokDefault = new Tokenizer(onmt::Tokenizer::mapMode.at("space"));
  }

  SubwordLearner::~SubwordLearner() {
    delete _pTokDefault;
  }

}
