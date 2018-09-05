/*
*/

#include "onmt/SPMLearner.h"
#include <fstream>
#include "onmt/Tokenizer.h"

#include "sentencepiece_trainer.h"
#include <iostream>
#include <cstdio>

namespace onmt
{
  SPMLearner::SPMLearner(bool verbose, std::string & opts, std::string inputfilename):
              SubwordLearner(verbose),
              _args(opts), _inputfilename(inputfilename), _tmpf(nullptr) {
    init_tempfile();
  }

  SPMLearner::SPMLearner(bool verbose, std::vector<std::string> & opts, std::string inputfilename):
              SubwordLearner(verbose),
              _args(""), _inputfilename(inputfilename), _tmpf(nullptr) {
    for(size_t i = 0; i < opts.size(); i+=2)
    {
      _args += opts[i] + "=" + opts[i+1] + " ";
    }

    init_tempfile();

  }

  void SPMLearner::init_tempfile()
  {
    if(_inputfilename.empty())
      _inputfilename = std::tmpnam(nullptr);

    _tmpf = std::fopen(_inputfilename.c_str(), "w");
  }

  void SPMLearner::ingest(std::istream &is, Tokenizer *pTok) {
    std::string text = "any text";

    std::fputs(text.c_str(), _tmpf);
  }

   void SPMLearner::learn(std::ostream &os, const char *description) {
    std::string outputfilename="sp_test";

    if(_tmpf)
      fclose(_tmpf);

    _args += " --input=" + _inputfilename;
    _args += " --model_prefix=" + outputfilename;

    sentencepiece::util::min_string_view args(_args);
    sentencepiece::SentencePieceTrainer::Train(args);
    std::cout << "INFO: If the process ends immediately after \"Parsing xxx ...\", check input parameters for SentencePiece" << std::endl;
    std::cout << _args << std::endl;
  }
}
