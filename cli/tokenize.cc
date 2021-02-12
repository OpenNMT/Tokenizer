#include <iostream>

#include <cxxopts.hpp>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>

#include "tokenization_args.h"

int main(int argc, char* argv[])
{
  cxxopts::Options cmd_options("tokenize");
  cmd_options.add_options()
    ("h,help", "Show this help")
    ("num_threads", "Number of threads to use",
     cxxopts::value<int>()->default_value("1"))
    ("seed", "Random seed for reproducible tokenization",
     cxxopts::value<unsigned int>()->default_value("0"))
    ("v,verbose", "Log tokenization progress",
     cxxopts::value<bool>()->default_value("false"))
    ;

  add_tokenization_options(cmd_options);

  cmd_options.add_options("Subword")
    ("bpe_model_path", "Path to the BPE model",
     cxxopts::value<std::string>()->default_value(""))
    ("b,bpe_model", "Aliases for --bpe_model_path",
     cxxopts::value<std::string>()->default_value(""))
    ("bpe_dropout", "Dropout BPE merge operations with this probability",
     cxxopts::value<float>()->default_value("0"))
    ("bpe_vocab", "Deprecated, see --vocabulary",
     cxxopts::value<std::string>()->default_value(""))
    ("bpe_vocab_threshold", "Deprecated, see --vocabulary_threshold",
     cxxopts::value<int>()->default_value("50"))

    ("sp_model_path", "Path to the SentencePiece model",
     cxxopts::value<std::string>()->default_value(""))
    ("s,sp_model", "Aliases for --sp_model_path",
     cxxopts::value<std::string>()->default_value(""))
    ("sp_nbest_size", "Number of candidates for the SentencePiece sampling API",
     cxxopts::value<int>()->default_value("0"))
    ("sp_alpha", "Smoothing parameter for the SentencePiece sampling API",
     cxxopts::value<float>()->default_value("0.1"))

    ("vocabulary", "If provided, sentences are encoded to subword present in this vocabulary",
     cxxopts::value<std::string>()->default_value(""))
    ("vocabulary_threshold",
     "If vocabulary is provided, any word with frequency < threshold will be treated as OOV.",
     cxxopts::value<int>()->default_value("0"))
    ;

  auto vm = cmd_options.parse(argc, argv);

  if (vm.count("help"))
  {
    std::cout << cmd_options.help() << std::endl;
    return 0;
  }

  if (vm.count("seed") != 0)
    onmt::set_random_seed(vm["seed"].as<unsigned int>());

  std::string vocabulary = vm["vocabulary"].as<std::string>();
  int vocabulary_threshold = vm["vocabulary_threshold"].as<int>();
  if (vocabulary.empty())
  {
    // Backward compatibility with previous option names.
    vocabulary = vm["bpe_vocab"].as<std::string>();
    vocabulary_threshold = vm["bpe_vocab_threshold"].as<int>();
  }

  onmt::SubwordEncoder* subword_encoder = nullptr;
  std::string bpe_model = (vm.count("bpe_model_path")
                           ? vm["bpe_model_path"].as<std::string>()
                           : vm["bpe_model"].as<std::string>());
  if (!bpe_model.empty())
    subword_encoder = new onmt::BPE(bpe_model,
                                    vm["bpe_dropout"].as<float>());
  else
  {
    std::string sp_model = (vm.count("sp_model_path")
                            ? vm["sp_model_path"].as<std::string>()
                            : vm["sp_model"].as<std::string>());
    if (!sp_model.empty())
      subword_encoder = new onmt::SentencePiece(sp_model,
                                                vm["sp_nbest_size"].as<int>(),
                                                vm["sp_alpha"].as<float>());
  }

  auto options = build_tokenization_options(vm);
  if (subword_encoder && !vocabulary.empty())
    subword_encoder->load_vocabulary(vocabulary, vocabulary_threshold, &options);

  onmt::Tokenizer tokenizer(std::move(options),
                            std::shared_ptr<onmt::SubwordEncoder>(subword_encoder));

  tokenizer.tokenize_stream(std::cin,
                            std::cout,
                            vm["num_threads"].as<int>(),
                            vm["verbose"].as<bool>());
  return 0;
}
