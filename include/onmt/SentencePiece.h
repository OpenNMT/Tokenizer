#pragma once

#include <string>

#include <sentencepiece_processor.h>

#include "onmt/SubwordEncoder.h"

namespace onmt
{

  class SentencePiece: public SubwordEncoder
  {
  public:
    SentencePiece(const std::string& model_path);

    void enable_regularization(int nbest_size, float alpha);

    std::vector<std::string> encode(const std::string& str) const override;
    std::vector<AnnotatedToken> encode_and_annotate(const AnnotatedToken& token) const override;

  private:
    sentencepiece::SentencePieceProcessor _processor;
    int _nbest_size;
    float _alpha;
  };

}
