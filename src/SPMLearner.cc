#include "onmt/SPMLearner.h"

#include <cstdio>
#include <iostream>

#include <sentencepiece_trainer.h>

#include "onmt/Tokenizer.h"

namespace onmt
{

  SPMLearner::SPMLearner(bool verbose,
                         const std::string& opts,
                         const std::string& input_filename)
    : SubwordLearner(verbose)
    , _args(opts)
    , _input_filename(input_filename)
  {
  }

  SPMLearner::SPMLearner(bool verbose,
                         const std::vector<std::string>& opts,
                         const std::string& input_filename)
    : SubwordLearner(verbose)
    , _input_filename(input_filename)
  {
    for(size_t i = 0; i < opts.size(); i += 2)
      _args += opts[i] + "=" + opts[i + 1] + " ";
  }

  SPMLearner::SPMLearner(bool verbose,
                         const std::unordered_map<std::string, std::string>& opts,
                         const std::string& input_filename)
    : SubwordLearner(verbose)
    , _input_filename(input_filename)
  {
    for (const auto& pair : opts)
      _args += " --" + pair.first + "=" + pair.second;
  }

  SPMLearner::~SPMLearner()
  {
    remove(_input_filename.c_str());
  }

  void SPMLearner::ingest(std::istream& is, const Tokenizer* tokenizer)
  {
    if (!_input_stream)
      _input_stream.reset(new std::ofstream(_input_filename));

    std::string line;
    while (std::getline(is, line))
    {
      if (!tokenizer)
        *_input_stream << line << std::endl;
      else
      {
        std::vector<AnnotatedToken> tokens;
        tokenizer->tokenize(line, tokens);
        for (const auto& token : tokens)
        {
          if (!Tokenizer::is_placeholder(token.str()))
            *_input_stream << token.str() << std::endl;
        }
      }
    }
  }

  void SPMLearner::learn(std::ostream& os, const char*)
  {
    std::string model_prefix = _input_filename + ".out";
    std::string sp_model_path = model_prefix + ".model";
    std::string sp_vocab_path = model_prefix + ".vocab";
    std::string final_args = _args;

    final_args += " --input=" + _input_filename;
    final_args += " --model_prefix=" + model_prefix;

    _input_stream.reset();

    if (!_verbose)
      std::cerr.setstate(std::ios_base::failbit);
    auto status = sentencepiece::SentencePieceTrainer::Train(final_args);
    if (!_verbose)
      std::cerr.clear();

    if (status.ok())
      os << std::ifstream(sp_model_path).rdbuf();

    remove(sp_model_path.c_str());
    remove(sp_vocab_path.c_str());
    remove(_input_filename.c_str());

    if (!status.ok())
      throw std::runtime_error("SentencePieceTrainer: " + status.ToString());
  }

}
