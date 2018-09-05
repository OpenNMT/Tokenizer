#pragma once

#include <string>
#include <fstream>
#include <memory>

#include "onmt/SubwordLearner.h"

namespace onmt
{

  class SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose, const std::string& opts, const std::string& input_filename);
    SPMLearner(bool verbose, const std::vector<std::string>& opts, const std::string& input_filename);

    void ingest(std::istream& is, Tokenizer* pTokenizer = 0) override;
    void learn(std::ostream& os, const char* description = 0) override;
  private:
    std::string _args;
    std::string _input_filename;
    std::unique_ptr<std::ofstream> _input_stream;
  };

}
