#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <unordered_map>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordLearner.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose, const std::string& opts, const std::string& input_filename);
    SPMLearner(bool verbose, const std::vector<std::string>& opts, const std::string& input_filename);
    SPMLearner(bool verbose,
               const std::unordered_map<std::string, std::string>& opts,
               const std::string& input_filename);

    void ingest(std::istream& is, Tokenizer* tokenizer = 0) override;
    void learn(std::ostream& os, const char* description = 0) override;
  private:
    std::string _args;
    std::string _input_filename;
    std::unique_ptr<std::ofstream> _input_stream;
  };

}
