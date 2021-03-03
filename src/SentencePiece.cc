#include "onmt/SentencePiece.h"

#include <sentencepiece_processor.h>
#include <stdexcept>

#include "Utils.h"

namespace onmt
{

  static const std::string sp_marker("▁");

  static inline void load_model(sentencepiece::SentencePieceProcessor& processor,
                                const std::string& model_path)
  {
    auto status = processor.Load(model_path);
    if (!status.ok())
      throw std::invalid_argument("Unable to open SentencePiece model " + model_path);
  }

  SentencePiece::SentencePiece(const std::string& model_path)
    : _processor(new sentencepiece::SentencePieceProcessor())
    , _nbest_size(0)
    , _alpha(0.0)
  {
    load_model(*_processor, model_path);
  }

  SentencePiece::SentencePiece(const std::string& model_path, int nbest_size, float alpha)
    : _processor(new sentencepiece::SentencePieceProcessor())
    , _nbest_size(nbest_size)
    , _alpha(alpha)
  {
    load_model(*_processor, model_path);
  }

  SentencePiece::~SentencePiece() = default;

  void SentencePiece::update_tokenization_options(Tokenizer::Options& options) const
  {
    // Maybe enable SentencePiece compatibility mode.
    if (options.mode == Tokenizer::Mode::None
        && !options.joiner_annotate
        && !options.spacer_annotate)
    {
      options.spacer_annotate = true;
      options.no_substitution = true;
    }
  }

  void SentencePiece::set_vocabulary(const std::vector<std::string>& vocabulary,
                                     const Tokenizer::Options* options)
  {
    if (options && (options->joiner_annotate || options->spacer_new))
      throw std::invalid_argument("SentencePiece vocabulary restriction requires the tokenization "
                                  "to use \"spacer_annotate\" (same as spm_encode)");
    auto status = _processor->SetVocabulary(vocabulary);
    if (!status.ok())
      throw std::invalid_argument(status.ToString());
  }

  void SentencePiece::reset_vocabulary()
  {
    _processor->ResetVocabulary();
  }

  void SentencePiece::enable_regularization(int nbest_size, float alpha)
  {
    _nbest_size = nbest_size;
    _alpha = alpha;
  }

  std::vector<std::string> SentencePiece::encode(const std::string& str, bool training) const
  {
    std::vector<std::string> pieces;

    if (training && _nbest_size != 0)
      _processor->SampleEncode(str, _nbest_size, _alpha, &pieces);
    else
      _processor->Encode(str, &pieces);

    return pieces;
  }

  std::vector<Token> SentencePiece::encode_and_annotate(const Token& token, bool training) const
  {
    std::vector<std::string> pieces = encode(token.surface, training);

    // SentencePiece sometimes returns no pieces for a non empty input. In this case
    // we simply return the original token.
    if (pieces.empty())
      return std::vector<Token>(1, token);

    std::vector<Token> tokens;
    tokens.reserve(pieces.size());
    bool apply_spacer_on_next = false;

    for (auto& piece : pieces)
    {
      // Prefixed by the spacer.
      if (starts_with(piece, sp_marker))
      {
        if (piece.length() == sp_marker.length())  // Piece is just the spacer.
        {
          // Skip this isolated spacer and mark the next piece with the spacer flag.
          apply_spacer_on_next = true;
          continue;
        }
        else
        {
          Token sub_token(piece.substr(sp_marker.length()));
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
