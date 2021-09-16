#include "onmt/SpaceTokenizer.h"

namespace onmt
{

  ITokenizer& SpaceTokenizer::get_instance()
  {
    static SpaceTokenizer tokenizer;
    return tokenizer;
  }

  void SpaceTokenizer::tokenize(const std::string& text,
                                std::vector<std::string>& words,
                                std::vector<std::vector<std::string> >& features,
                                bool) const
  {
    read_tokens(text, words, features);
  }

  std::string SpaceTokenizer::detokenize(const std::vector<std::string>& words,
                                         const std::vector<std::vector<std::string> >& features) const
  {
    return write_tokens(words, features);
  }

}
