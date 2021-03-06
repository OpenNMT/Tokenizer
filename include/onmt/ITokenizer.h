#pragma once

#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

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
                          std::vector<std::vector<std::string> >& features,
                          bool training = true) const = 0;
    virtual void tokenize(const std::string& text,
                          std::vector<std::string>& words,
                          std::vector<std::vector<std::string> >& features,
                          std::unordered_map<std::string, size_t>& alphabets,
                          bool training = true) const;
    virtual void tokenize(const std::string& text,
                          std::vector<std::string>& words,
                          bool training = true) const;

    virtual std::string detokenize(const std::vector<std::string>& words,
                                   const std::vector<std::vector<std::string> >& features) const = 0;
    virtual std::string detokenize(const std::vector<std::string>& words) const;
    virtual std::string detokenize(const std::vector<std::string>& words,
                                   const std::vector<std::vector<std::string> >& features,
                                   Ranges& ranges, bool merge_ranges = false) const;
    virtual std::string detokenize(const std::vector<std::string>& words,
                                   Ranges& ranges, bool merge_ranges = false) const;

    virtual void tokenize_stream(std::istream& is,
                                 std::ostream& os,
                                 size_t num_threads = 1,
                                 bool verbose = false,
                                 bool training = true,
                                 size_t buffer_size = 1000) const;

    virtual void detokenize_stream(std::istream& is, std::ostream& os) const;
  };

}
