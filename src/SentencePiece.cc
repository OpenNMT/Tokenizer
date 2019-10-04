#include "onmt/SentencePiece.h"

#include <sentencepiece_processor.h>
#include <stdexcept>

namespace onmt
{

  static const std::string sp_marker("▁");

  class SentencePieceProcessor : public sentencepiece::SentencePieceProcessor
  {
  };

  static void load_model(SentencePieceProcessor& processor, const std::string& model_path)
  {
    auto status = processor.Load(model_path);
    if (!status.ok())
      throw std::invalid_argument("Unable to open SentencePiece model " + model_path);
  }

  SentencePiece::SentencePiece(const std::string& model_path)
    : _processor(new SentencePieceProcessor())
    , _nbest_size(0)
    , _alpha(0.0)
  {
    load_model(*_processor, model_path);
  }

  SentencePiece::SentencePiece(const std::string& model_path, int nbest_size, float alpha)
    : _processor(new SentencePieceProcessor())
    , _nbest_size(nbest_size)
    , _alpha(alpha)
  {
    load_model(*_processor, model_path);
  }

  SentencePiece::~SentencePiece()
  {
    delete _processor;
  }

  void SentencePiece::set_vocabulary(const std::vector<std::string>& vocabulary)
  {
#ifdef SP_HAS_VOCAB_RESTRICTION
    auto status = _processor->SetVocabulary(vocabulary);
    if (!status.ok())
      throw std::invalid_argument(status.ToString());
#else
    throw std::runtime_error("The project was built against a SentencePiece version "
                             "that does not support vocabulary restriction");
#endif
  }

  void SentencePiece::reset_vocabulary()
  {
#ifdef SP_HAS_VOCAB_RESTRICTION
    _processor->ResetVocabulary();
#else
    throw std::runtime_error("The project was built against a SentencePiece version "
                             "that does not support vocabulary restriction");
#endif
  }

  void SentencePiece::enable_regularization(int nbest_size, float alpha)
  {
    _nbest_size = nbest_size;
    _alpha = alpha;
  }

  std::vector<std::string> SentencePiece::encode(const std::string& str) const
  {
    std::vector<std::string> pieces;

#ifdef SP_HAS_SAMPLE_ENCODE
    if (_nbest_size != 0)
      _processor->SampleEncode(str, _nbest_size, _alpha, &pieces);
    else
#endif
    {
      _processor->Encode(str, &pieces);
    }

    return pieces;
  }

  std::vector<AnnotatedToken> SentencePiece::encode_and_annotate(const AnnotatedToken& token) const
  {
    std::vector<std::string> encoded = encode(token.str());
    std::vector<AnnotatedToken> tokens;
    if (encoded.empty())
    {
      tokens.push_back(token);
      return tokens;
    }

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

      if (j > 0 && !is_marked)
        new_token.join_left();
      if (is_marked)
        new_token.spacer();
    }

    propagate_token_properties(token, tokens);
    return tokens;
  }

}
