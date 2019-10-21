#include "onmt/SubwordLearner.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace onmt
{

  SubwordLearner::SubwordLearner(bool verbose, const Tokenizer* default_tokenizer)
    : _verbose(verbose)
    , _default_tokenizer(default_tokenizer
                         ? default_tokenizer
                         : new Tokenizer(Tokenizer::Mode::None, Tokenizer::Flags::NoSubstitution))
  {
  }

  void SubwordLearner::ingest(const std::string& text, const Tokenizer* tokenizer)
  {
    if (!tokenizer)
      tokenizer = _default_tokenizer.get();

    std::vector<AnnotatedToken> tokens;
    tokenizer->tokenize(text, tokens);
    for (const auto& token : tokens)
    {
      if (!Tokenizer::is_placeholder(token.str()))
        ingest_token(token.str());
    }
  }

  void SubwordLearner::ingest(std::istream& is, const Tokenizer* tokenizer)
  {
    std::string line;
    while (std::getline(is, line))
      ingest(line, tokenizer);
  }

  void SubwordLearner::learn(const std::string& model_path, const char* description, bool verbose)
  {
    std::ofstream out(model_path);
    if (!out)
      throw std::invalid_argument("Failed to open model path " + model_path);
    learn(out, description, verbose);
  }

}
