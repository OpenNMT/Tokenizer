#pragma once

#include "opennmttokenizer_export.h"
#include "onmt/ITokenizer.h"

namespace onmt
{

  // This Tokenizer simply splits on spaces. Useful when the text was tokenized
  // with an external tool.
  class OPENNMTTOKENIZER_EXPORT SpaceTokenizer: public ITokenizer
  {
  public:
    static ITokenizer& get_instance();

    void tokenize(const std::string& text,
                  std::vector<std::string>& words,
                  std::vector<std::vector<std::string> >& features) const override;

    std::string detokenize(const std::vector<std::string>& words,
                           const std::vector<std::vector<std::string> >& features) const override;

  };

}
