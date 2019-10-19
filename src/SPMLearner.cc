#include "onmt/SPMLearner.h"

#include <sentencepiece_trainer.h>

namespace onmt
{

  SPMLearner::SPMLearner(bool verbose,
                         const std::string& opts,
                         const std::string& input_filename,
                         bool keep_vocab)
    : SubwordLearner(verbose)
    , _args(opts)
    , _input_filename(input_filename)
    , _keep_vocab(keep_vocab)
  {
  }

  SPMLearner::SPMLearner(bool verbose,
                         const std::vector<std::string>& opts,
                         const std::string& input_filename,
                         bool keep_vocab)
    : SubwordLearner(verbose)
    , _input_filename(input_filename)
    , _keep_vocab(keep_vocab)
  {
    for(size_t i = 0; i < opts.size(); i += 2)
      _args += opts[i] + "=" + opts[i + 1] + " ";
  }

  SPMLearner::SPMLearner(bool verbose,
                         const std::unordered_map<std::string, std::string>& opts,
                         const std::string& input_filename,
                         bool keep_vocab)
    : SubwordLearner(verbose)
    , _input_filename(input_filename)
    , _keep_vocab(keep_vocab)
  {
    for (const auto& pair : opts)
      _args += " --" + pair.first + "=" + pair.second;
  }

  SPMLearner::~SPMLearner()
  {
    remove(_input_filename.c_str());
  }

  void SPMLearner::set_input_filename(const std::string& filename)
  {
    if (_input_stream)
      _input_stream.reset();
    _input_filename = filename;
  }

  void SPMLearner::init_input_stream()
  {
    if (!_input_stream)
      _input_stream.reset(new std::ofstream(_input_filename));
  }

  void SPMLearner::ingest_token(const std::string& token)
  {
    init_input_stream();
    *_input_stream << token << std::endl;
  }

  void SPMLearner::learn(std::ostream& os, const char* description, bool verbose)
  {
    if (_keep_vocab)
      throw std::invalid_argument("stream API does not support keeping the SentencePiece vocabulary");
    std::string model_path = _input_filename + ".out";
    learn(model_path, description, verbose);

    // Move the model content into the output stream.
    os << std::ifstream(model_path).rdbuf();
    remove(model_path.c_str());
  }

  void SPMLearner::learn(const std::string& model_path, const char*, bool verbose)
  {
    verbose = verbose || _verbose;
    _input_stream.reset();  // Freeze the input file for training.

    if (!verbose)
      std::cerr.setstate(std::ios_base::failbit);
    auto status = sentencepiece::SentencePieceTrainer::Train(_args
                                                             + " --input=" + _input_filename
                                                             + " --model_prefix=" + model_path);
    if (!verbose)
      std::cerr.clear();

    // Cleanup the input file.
    remove(_input_filename.c_str());

    std::string sp_model_path = model_path + ".model";
    std::string sp_vocab_path = model_path + ".vocab";

    if (!status.ok())
    {
      // Cleanup the output files on error.
      remove(sp_model_path.c_str());
      remove(sp_vocab_path.c_str());
      throw std::runtime_error("SentencePieceTrainer: " + status.ToString());
    }
    else if (!_keep_vocab)
    {
      // Remove the generated vocabulary and move the model to the request path.
      rename(sp_model_path.c_str(), model_path.c_str());
      remove(sp_vocab_path.c_str());
    }
  }

}
