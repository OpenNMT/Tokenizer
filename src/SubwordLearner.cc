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

  void SubwordLearner::ingest_token(const Token& token)
  {
    if (!token.empty() && !token.is_placeholder())
      ingest_token_impl(token.surface);
  }

  void SubwordLearner::ingest_token(const std::string& token, const Tokenizer* tokenizer)
  {
    if (!tokenizer)
      tokenizer = _default_tokenizer.get();
    ingest_token(tokenizer->annotate_token(token));
  }

  void SubwordLearner::ingest(const std::string& text, const Tokenizer* tokenizer)
  {
    if (!tokenizer)
      tokenizer = _default_tokenizer.get();

    std::vector<Token> tokens;
    tokenizer->tokenize(text, tokens);
    for (const auto& token : tokens)
      ingest_token(token);
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

  const std::shared_ptr<const Tokenizer>& SubwordLearner::get_default_tokenizer() const
  {
    return _default_tokenizer;
  }

}
