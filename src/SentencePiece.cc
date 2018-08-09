#include "onmt/SentencePiece.h"
#include <json/value.h>
#include <json/json.h>
#include <fstream>

namespace onmt
{

  static const std::string sp_marker("▁");

  SentencePiece::SentencePiece(const std::string& model_path)
    :sp_nbest_size(0),sp_alpha(0.0),sp_subword_regularization(false)
  {
    try
    {
      std::ifstream model_file(model_path, std::ifstream::binary);
      Json::Value model_json;
      model_file >> model_json;

      std::string sp_model_path = model_json["model"].asString();
      sp_nbest_size = model_json["nbest_size"].asInt();
      sp_alpha = model_json["alpha"].asDouble();

      _processor.Load(sp_model_path);
      sp_subword_regularization = true;
    }
    catch (...)
    {
      _processor.Load(model_path);
    }
  }

  std::vector<std::string> SentencePiece::encode(const std::string& str) const
  {
    std::vector<std::string> pieces;
    if(sp_subword_regularization)
      _processor.SampleEncode(str, sp_nbest_size, sp_alpha, &pieces);
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
