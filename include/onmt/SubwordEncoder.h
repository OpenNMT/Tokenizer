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

    virtual std::vector<std::string> encode(const std::string& str) const = 0;
    virtual std::vector<AnnotatedToken> encode_and_annotate(const AnnotatedToken& token) const;
  };

}
