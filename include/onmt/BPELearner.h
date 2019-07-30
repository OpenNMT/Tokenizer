#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordLearner.h"

namespace onmt
{
  class OPENNMTTOKENIZER_EXPORT BPELearner: public SubwordLearner
  {
  public:
    BPELearner(bool verbose,
               int symbols, int min_frequency, bool dict_input, bool total_symbols);
    void ingest(std::istream& is, const Tokenizer* tokenizer = 0);
    void learn(std::ostream& os, const char* description = 0, bool verbose = false);
  private:
    int _symbols;
    int _min_frequency;
    bool _dict_input;
    bool _total_symbols;
    std::unordered_map<std::string, int> _vocab;
    std::unique_ptr<const Tokenizer> _default_tokenizer;
  };

}
