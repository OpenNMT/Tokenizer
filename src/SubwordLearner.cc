#include "onmt/SubwordLearner.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace onmt
{

  SubwordLearner::SubwordLearner(bool verbose)
    : _verbose(verbose)
  {
  }

  void SubwordLearner::ingest(const std::string& text, const Tokenizer* tokenizer)
  {
    std::istringstream in(text);
    ingest(in, tokenizer);
  }

  void SubwordLearner::learn(const std::string& model_path, const char* description, bool verbose)
  {
    std::ofstream out(model_path);
    if (!out)
      throw std::invalid_argument("Failed to open model path " + model_path);
    learn(out, description, verbose);
  }

}
