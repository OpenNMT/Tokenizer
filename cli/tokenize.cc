#include <iostream>

#include <boost/program_options.hpp>

#include <onmt/Tokenizer.h>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::options_description desc("Tokenization");
  desc.add_options()
    ("help", "display available options")
    ("mode", po::value<std::string>()->default_value("conservative"), "Define how aggressive should the tokenization be: 'aggressive' only keeps sequences of letters/numbers, 'conservative' allows mix of alphanumeric as in: '2,000', 'E65', 'soft-landing'")
    ("joiner_annotate", po::bool_switch()->default_value(false), "include joiner annotation using 'joiner' character")
    ("joiner", po::value<std::string>()->default_value(onmt::Tokenizer::joiner_marker), "character used to annotate joiners")
    ("joiner_new", po::bool_switch()->default_value(false), "in joiner_annotate mode, 'joiner' is an independent token")
    ("case_feature", po::bool_switch()->default_value(false), "lowercase corpus and generate case feature")
    ("bpe_model", po::value<std::string>()->default_value(""), "path to the BPE model")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cerr << desc << std::endl;
    return 1;
  }

  onmt::ITokenizer* tokenizer = new onmt::Tokenizer(vm["mode"].as<std::string>() == "aggressive"
                                                    ? onmt::Tokenizer::Mode::Aggressive
                                                    : onmt::Tokenizer::Mode::Conservative,
                                                    vm["bpe_model"].as<std::string>(),
                                                    vm["case_feature"].as<bool>(),
                                                    vm["joiner_annotate"].as<bool>(),
                                                    vm["joiner_new"].as<bool>(),
                                                    vm["joiner"].as<std::string>());

  std::string line;

  while (std::getline(std::cin, line))
  {
    if (!line.empty())
      std::cout << tokenizer->tokenize(line);

    std::cout << std::endl;
  }

  return 0;
}
