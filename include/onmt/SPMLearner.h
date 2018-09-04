#pragma once

#include <string>

#include "onmt/SubwordLearner.h"
#include "trainer_interface.h"

namespace onmt
{
  class SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose, Tokenizer *pTokenizer,
               int argc, char **argv);
    void ingest(std::istream &is);
    void learn(std::ostream &os);
  private:
    int _argc;
    char **_argv;
    std::unique_ptr<sentencepiece::TrainerInterface> _trainer;
  };

}
