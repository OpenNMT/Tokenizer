#include "onmt/SPMLearner.h"

#include <fstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>

#include <sentencepiece_trainer.h>

#include "onmt/Tokenizer.h"

namespace onmt
{

  SPMLearner::SPMLearner(bool verbose, std::string & opts, std::string input_filename)
    : SubwordLearner(verbose)
    , _args(opts)
    , _input_filename(input_filename)
    , _tmpf(nullptr)
  {
    init_tempfile();
  }

  SPMLearner::SPMLearner(bool verbose, std::vector<std::string> & opts, std::string input_filename)
    : SubwordLearner(verbose)
    , _args("")
    , _input_filename(input_filename)
    , _tmpf(nullptr)
  {
    for(size_t i = 0; i < opts.size(); i += 2)
      _args += opts[i] + "=" + opts[i + 1] + " ";

    init_tempfile();
  }

  void SPMLearner::init_tempfile()
  {
    if (_input_filename.empty())
      _input_filename = std::tmpnam(nullptr);

    _tmpf = std::fopen(_input_filename.c_str(), "w");
  }

  void SPMLearner::ingest(std::istream& is, Tokenizer*)
  {
    std::string text = "any text";
    std::fputs(text.c_str(), _tmpf);
  }

  void SPMLearner::learn(std::ostream&, const char*)
  {
    throw std::runtime_error("SPMLearner is not compatible with stream output");
  }

  void SPMLearner::learn(const std::string& output_filename, const char*)
  {
    if (_tmpf)
      fclose(_tmpf);

    _args += " --input=" + _input_filename;
    _args += " --model_prefix=" + output_filename;

    sentencepiece::util::min_string_view args(_args);
    sentencepiece::SentencePieceTrainer::Train(args);
    std::cout << "INFO: If the process ends immediately after \"Parsing xxx ...\", check input parameters for SentencePiece" << std::endl;
    std::cout << _args << std::endl;
  }

}
