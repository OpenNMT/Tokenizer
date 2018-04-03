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
      if (j == 0 && token.is_joined_left())
        tokens.back().join_left();
      if (j + 1 < encoded.size() || token.is_joined_right())
        tokens.back().join_right();
    }

    return tokens;
  }

}
