#pragma once

#include <memory>
#include <string>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordEncoder.h"

namespace sentencepiece
{
  class SentencePieceProcessor;
}

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT SentencePiece: public SubwordEncoder
  {
  public:
    SentencePiece(const std::string& model_path);
    SentencePiece(const std::string& model_path, int nbest_size, float alpha);
    ~SentencePiece();

    void update_tokenization_options(Tokenizer::Options& options) const override;
    void set_vocabulary(const std::vector<std::string>& vocabulary,
                        const Tokenizer::Options* options = nullptr) override;
    void reset_vocabulary() override;
    void enable_regularization(int nbest_size, float alpha);

    std::vector<std::string> encode(const std::string& str, bool training = true) const override;
    std::vector<Token> encode_and_annotate(const Token& token, bool training = true) const override;

  private:
    const std::unique_ptr<sentencepiece::SentencePieceProcessor> _processor;
    int _nbest_size;
    float _alpha;
  };

}
