#pragma once

#include <vector>
#include <string>

namespace onmt
{

  class ITokenizer
  {
  public:
    static const std::string feature_marker;

    virtual ~ITokenizer() = default;

    virtual void tokenize(const std::string& text,
                          std::vector<std::string>& words,
                          std::vector<std::vector<std::string> >& features) = 0;
    virtual void tokenize(const std::string& text, std::vector<std::string>& words);

    virtual std::string detokenize(const std::vector<std::string>& words,
                                   const std::vector<std::vector<std::string> >& features) = 0;
    virtual std::string detokenize(const std::vector<std::string>& words);

    // Tokenize and use spaces as token separators.
    virtual std::string tokenize(const std::string& text);

    // Split the text on spaces and detokenize.
    virtual std::string detokenize(const std::string& text);
  };

}
