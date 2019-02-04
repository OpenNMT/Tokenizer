#pragma once

#include <map>
#include <vector>
#include <string>
#include <unordered_map>

#include "onmt/opennmttokenizer_export.h"

namespace onmt
{

  using Range = std::pair<size_t, size_t>;
  using Ranges = std::map<size_t, Range>;

  class OPENNMTTOKENIZER_EXPORT ITokenizer
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
    virtual std::string detokenize(const std::vector<std::string>& words,
                                   const std::vector<std::vector<std::string> >& features,
                                   Ranges& ranges, bool merge_ranges = false) const;
    virtual std::string detokenize(const std::vector<std::string>& words,
                                   Ranges& ranges, bool merge_ranges = false) const;

    // Tokenize and use spaces as token separators.
    virtual std::string tokenize(const std::string& text) const;

    // Split the text on spaces and detokenize.
    virtual std::string detokenize(const std::string& text) const;
  };

}
