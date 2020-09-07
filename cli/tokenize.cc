#include <iostream>

#include <cxxopts.hpp>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#ifdef WITH_SP
#  include <onmt/SentencePiece.h>
#endif

#include "tokenization_args.h"

int main(int argc, char* argv[])
{
  cxxopts::Options cmd_options("tokenize");
  cmd_options.add_options()
    ("h,help", "Show this help")
    ("num_threads", "Number of threads to use",
     cxxopts::value<int>()->default_value("1"))
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

#ifdef WITH_SP
    ("sp_model_path", "Path to the SentencePiece model",
     cxxopts::value<std::string>()->default_value(""))
    ("s,sp_model", "Aliases for --sp_model_path",
     cxxopts::value<std::string>()->default_value(""))
    ("sp_nbest_size", "Number of candidates for the SentencePiece sampling API",
     cxxopts::value<int>()->default_value("0"))
    ("sp_alpha", "Smoothing parameter for the SentencePiece sampling API",
     cxxopts::value<float>()->default_value("0.1"))
#endif

    ("vocabulary", "If provided, sentences are encoded to subword present in this vocabulary",
     cxxopts::value<std::string>()->default_value(""))
    ("vocabulary_threshold",
     "If vocabulary is provided, any word with frequency < threshold will be treated as OOV.",
     cxxopts::value<int>()->default_value("0"))
    ;

  auto vm = cmd_options.parse(argc, argv);

  if (vm.count("help"))
  {
    std::cerr << cmd_options.help() << std::endl;
    return 0;
  }

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
                                    vm["joiner"].as<std::string>(),
                                    vm["bpe_dropout"].as<float>());
#ifdef WITH_SP
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
#endif

  if (subword_encoder && !vocabulary.empty())
    subword_encoder->load_vocabulary(vocabulary, vocabulary_threshold);

  onmt::Tokenizer tokenizer(onmt::Tokenizer::str_to_mode(vm["mode"].as<std::string>()),
                            subword_encoder,
                            build_tokenization_flags(vm),
                            vm["joiner"].as<std::string>());

  for (const auto& alphabet : vm["segment_alphabet"].as<std::vector<std::string>>())
  {
    if (!alphabet.empty() && !tokenizer.add_alphabet_to_segment(alphabet))
      std::cerr << "WARNING: " << alphabet << " alphabet is not supported" << std::endl;
  }

  tokenizer.tokenize_stream(std::cin, std::cout, vm["num_threads"].as<int>());
  return 0;
}
