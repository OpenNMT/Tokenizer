#include "onmt/SubwordLearner.h"
#include <onmt/Tokenizer.h>

namespace onmt
{

  SubwordLearner::SubwordLearner(bool verbose, Tokenizer *pTok):
        _verbose(verbose), _pTok(pTok) {
    if (!_pTok)
      pTok = new Tokenizer(onmt::Tokenizer::mapMode.at("space"));
    else
      pTok->unset_annotate();
  }

}
