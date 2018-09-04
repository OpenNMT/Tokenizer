#pragma once

#include <string>

#include "onmt/SubwordLearner.h"
#include "trainer_interface.h"

namespace onmt
{
  class SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose,
               int argc, char **argv);
    void ingest(std::istream &is, Tokenizer *pTokenizer=0);
    void learn(std::ostream &os, const char *description=0);
  private:
    int _argc;
    char **_argv;
    std::unique_ptr<sentencepiece::TrainerInterface> _trainer;
  };

}
