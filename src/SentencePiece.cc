#include "onmt/SentencePiece.h"

#include <sentencepiece_processor.h>
#include <stdexcept>

namespace onmt
{

  static const std::string sp_marker("▁");
  static const auto sp_marker_length = sp_marker.length();

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

  std::vector<Token> SentencePiece::encode_and_annotate(const Token& token) const
  {
    std::vector<std::string> pieces = encode(token.surface);

    // SentencePiece sometimes returns no pieces for a non empty input. In this case
    // we simply return the original token.
    if (pieces.empty())
      return std::vector<Token>(1, token);

    std::vector<Token> tokens;
    tokens.reserve(pieces.size());
    bool apply_spacer_on_next = false;

    for (auto& piece : pieces)
    {
      const auto piece_length = piece.length();

      // Prefixed by the spacer.
      if (piece_length >= sp_marker_length && piece.compare(0, sp_marker_length, sp_marker) == 0)
      {
        if (piece_length == sp_marker_length)  // Piece is just the spacer.
        {
          // Skip this isolated spacer and mark the next piece with the spacer flag.
          apply_spacer_on_next = true;
          continue;
        }
        else
        {
          Token sub_token(piece.substr(sp_marker_length));
          sub_token.spacer = true;
          tokens.emplace_back(std::move(sub_token));
        }
      }
      else
      {
        Token sub_token(std::move(piece));
        if (apply_spacer_on_next)
        {
          sub_token.spacer = true;
          sub_token.preserve = true;  // The spacer was not attached to this piece so preserve it.
          apply_spacer_on_next = false;
        }
        else if (!tokens.empty())
        {
          sub_token.join_left = true;  // No spacer means it should be joined with the previous subtoken.
        }
        tokens.emplace_back(std::move(sub_token));
      }
    }

    propagate_token_properties(token, tokens);
    return tokens;
  }

}
