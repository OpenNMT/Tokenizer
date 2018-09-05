#pragma once

#include <string>

#include "onmt/SubwordLearner.h"

namespace onmt
{
  class SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose, std::string & opts, std::string inputfilename="");
    SPMLearner(bool verbose, std::vector<std::string> & opts, std::string inputfilename="");

    void ingest(std::istream &is, Tokenizer *pTokenizer=0);
    void learn(std::ostream &os, const char *description=0);
  private:
    std::string _args;

    std::string _inputfilename;
    std::FILE* _tmpf;

    void init_tempfile();
  };

}
