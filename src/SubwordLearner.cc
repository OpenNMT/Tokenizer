#include "onmt/SubwordLearner.h"
#include <onmt/Tokenizer.h>

namespace onmt
{

  SubwordLearner::SubwordLearner(bool verbose)
    : _verbose(verbose)
    , _default_tokenizer(new Tokenizer(onmt::Tokenizer::mapMode.at("space")))
  {
  }

}
