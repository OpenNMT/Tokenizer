#pragma once

#include <string>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordEncoder.h"

namespace onmt
{

  // Use the PImpl idiom to hide the sentencepiece dependency.
  class OPENNMTTOKENIZER_EXPORT SentencePieceProcessor;

  class OPENNMTTOKENIZER_EXPORT SentencePiece: public SubwordEncoder
  {
  public:
    SentencePiece(const std::string& model_path);
    SentencePiece(const std::string& model_path, int nbest_size, float alpha);
    ~SentencePiece();

    void set_vocabulary(const std::vector<std::string>& vocabulary) override;
    void reset_vocabulary() override;
    void enable_regularization(int nbest_size, float alpha);

    std::vector<std::string> encode(const std::string& str) const override;
    std::vector<Token> encode_and_annotate(const Token& token) const override;

  private:
    SentencePieceProcessor* _processor;
    int _nbest_size;
    float _alpha;
  };

}
