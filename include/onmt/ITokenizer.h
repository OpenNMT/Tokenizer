#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace onmt
{

  class ITokenizer
  {
  public:
    static const std::string feature_marker;

    virtual ~ITokenizer() = default;

    virtual void tokenize(const std::string& text,
                          std::vector<std::string>& words,
                          std::vector<std::vector<std::string> >& features) const = 0;
    virtual void tokenize(const std::string& text,
                          std::vector<std::string>& words,
                          std::vector<std::vector<std::string> >& features,
                          std::unordered_map<std::string, size_t>& alphabets) const;
    virtual void tokenize(const std::string& text, std::vector<std::string>& words) const;

    virtual std::string detokenize(const std::vector<std::string>& words,
                                   const std::vector<std::vector<std::string> >& features) const = 0;
    virtual std::string detokenize(const std::vector<std::string>& words) const;

    // Tokenize and use spaces as token separators.
    virtual std::string tokenize(const std::string& text) const;

    // Split the text on spaces and detokenize.
    virtual std::string detokenize(const std::string& text) const;
  };

}
