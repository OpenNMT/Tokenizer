#include "onmt/SentencePiece.h"

namespace onmt
{

  static const std::string sp_marker("▁");

  SentencePiece::SentencePiece(const std::string& model_path)
    : _nbest_size(0)
    , _alpha(0.0)
  {
    _processor.Load(model_path);
  }

  void SentencePiece::enable_regularization(int nbest_size, double alpha)
  {
    _nbest_size = nbest_size;
    _alpha = alpha;
  }

  std::vector<std::string> SentencePiece::encode(const std::string& str) const
  {
    std::vector<std::string> pieces;

    if (_nbest_size != 0)
      _processor.SampleEncode(str, _nbest_size, _alpha, &pieces);
    else
      _processor.Encode(str, &pieces);

    return pieces;
  }

  std::vector<AnnotatedToken> SentencePiece::encode_and_annotate(const AnnotatedToken& token) const
  {
    std::vector<std::string> encoded = encode(token.str());
    std::vector<AnnotatedToken> tokens;
    tokens.reserve(encoded.size());

    for (size_t j = 0; j < encoded.size(); ++j)
    {
      const auto& piece = encoded[j];
      const bool is_marked = (piece.length() >= sp_marker.length()
                              && piece.compare(0, sp_marker.length(), sp_marker) == 0);

      tokens.emplace_back();
      auto& new_token = tokens.back();

      if (is_marked)
        new_token.set(piece.substr(sp_marker.length()));
      else
        new_token.set(std::move(piece));

      if ((j == 0 && token.is_joined_left()) || (j > 0 && !is_marked))
        new_token.join_left();
      if (j + 1 == encoded.size() && token.is_joined_right())
        new_token.join_right();
      if (is_marked)
        new_token.spacer();
    }

    return tokens;
  }

}
