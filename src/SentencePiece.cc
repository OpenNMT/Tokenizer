#include "onmt/SentencePiece.h"

namespace onmt
{

  static const std::string sp_marker("▁");

  SentencePiece::SentencePiece(const std::string& model_path)
  {
    _processor.Load(model_path);
  }

  std::vector<std::string> SentencePiece::encode(const std::string& str) const
  {
    std::vector<std::string> pieces;
    _processor.Encode(str, &pieces);
    return pieces;
  }

  std::vector<AnnotatedToken> SentencePiece::encode_and_annotate(const AnnotatedToken& token) const
  {
    std::vector<std::string> encoded = encode(token.str());
    std::vector<AnnotatedToken> tokens;

    for (size_t j = 0; j < encoded.size(); ++j)
    {
      std::string piece = encoded[j];
      bool is_marked = piece.find(sp_marker) == 0;
      if (is_marked)
        piece.erase(0, sp_marker.length());
      AnnotatedToken new_token(piece);
      if ((j == 0 && token.is_joined_left()) || (j > 0 && !is_marked))
        new_token.join_left();
      if (j + 1 == encoded.size() && token.is_joined_right())
        new_token.join_right();
      if (is_marked)
        new_token.spacer();
      tokens.push_back(new_token);
    }

    return tokens;
  }

}
