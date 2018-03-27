#pragma once

#include "onmt/ITokenizer.h"

namespace onmt
{

  // This Tokenizer simply splits on spaces. Useful when the text was tokenized
  // with an external tool.
  class SpaceTokenizer: public ITokenizer
  {
  public:
    static ITokenizer& get_instance();

    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features) const override;
    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features,
                  std::unordered_map<std::string,size_t>& alphabets) const override;

    std::string detokenize(const std::vector<std::string>& words,
                           const std::vector<std::vector<std::string> >& features) const override;

  };

}
