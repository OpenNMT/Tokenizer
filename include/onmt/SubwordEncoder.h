#pragma once

#include <string>
#include <vector>

#include "onmt/AnnotatedToken.h"

namespace onmt
{

  class SubwordEncoder
  {
  public:
    virtual ~SubwordEncoder() = default;

    virtual void load_vocabulary(const std::string& path, int frequency_threshold);
    virtual void set_vocabulary(const std::vector<std::string>& vocabulary);
    virtual void reset_vocabulary();

    virtual std::vector<std::string> encode(const std::string& str) const = 0;
    virtual std::vector<AnnotatedToken> encode_and_annotate(const AnnotatedToken& token) const;
  };

}
