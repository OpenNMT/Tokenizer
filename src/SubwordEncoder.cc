#include "onmt/SubwordEncoder.h"

namespace onmt
{

  std::vector<AnnotatedToken> SubwordEncoder::encode_and_annotate(const AnnotatedToken& token) const
  {
    std::vector<std::string> encoded = encode(token.str());
    std::vector<AnnotatedToken> tokens;

    for (size_t j = 0; j < encoded.size(); ++j)
    {
      tokens.emplace_back(encoded[j]);
      if (j + 1 < encoded.size())
        tokens.back().join_right();
    }

    if (token.is_joined_left())
    {
      tokens.front().join_left();
      if (token.should_preserve())
        tokens.front().preserve();
    }
    if (token.is_joined_right())
    {
      tokens.back().join_right();
      if (token.should_preserve())
        tokens.back().preserve();
    }



    return tokens;
  }

}
