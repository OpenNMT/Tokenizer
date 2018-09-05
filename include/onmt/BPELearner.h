#pragma once

#include <string>
#include <unordered_map>

#include "onmt/SubwordLearner.h"

namespace onmt
{
  class BPELearner: public SubwordLearner
  {
  public:
    BPELearner(bool verbose,
               int symbols, int min_frequency, bool dict_input, bool total_symbols);

    void ingest(std::istream& is, Tokenizer* pTokenizer = 0) override;
    void learn(std::ostream& os, const char* description = 0) override;
    void learn(const std::string& output_filename, const char* description = 0) override;
  private:
    int _symbols;
    int _min_frequency;
    bool _dict_input;
    bool _total_symbols;
    std::unordered_map<std::string, int> _vocab;
  };

}
