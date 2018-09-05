#pragma once

#include <string>

#include "onmt/SubwordLearner.h"

namespace onmt
{

  class SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose, std::string & opts, std::string input_filename="");
    SPMLearner(bool verbose, std::vector<std::string> & opts, std::string input_filename="");

    void ingest(std::istream& is, Tokenizer* pTokenizer = 0) override;
    void learn(std::ostream& os, const char* description = 0) override;
    void learn(const std::string& output_filename, const char* description = 0) override;
  private:
    std::string _args;

    std::string _input_filename;
    std::FILE* _tmpf;

    void init_tempfile();
  };

}
