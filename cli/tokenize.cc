#include <iostream>

#include <boost/program_options.hpp>

#include <onmt/Tokenizer.h>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::options_description desc("Tokenization");
  desc.add_options()
    ("help,h", "display available options")
    ("mode,m", po::value<std::string>()->default_value("conservative"), "Define how aggressive should the tokenization be: 'aggressive' only keeps sequences of letters/numbers, 'conservative' allows mix of alphanumeric as in: '2,000', 'E65', 'soft-landing'")
    ("joiner_annotate,j", po::bool_switch()->default_value(false), "include joiner annotation using 'joiner' character")
    ("joiner", po::value<std::string>()->default_value(onmt::Tokenizer::joiner_marker), "character used to annotate joiners")
    ("joiner_new", po::bool_switch()->default_value(false), "in joiner_annotate mode, 'joiner' is an independent token")
    ("case_feature,c", po::bool_switch()->default_value(false), "lowercase corpus and generate case feature")
    ("segment_case", po::bool_switch()->default_value(false), "Segment case feature, splits AbC to Ab C to be able to restore case")
    ("segment_numbers", po::bool_switch()->default_value(false), "Segment numbers into single digits")
    ("bpe_model,bpe", po::value<std::string>()->default_value(""), "path to the BPE model")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cerr << desc << std::endl;
    return 1;
  }

  onmt::ITokenizer* tokenizer = new onmt::Tokenizer(onmt::Tokenizer::mapMode.at(vm["mode"].as<std::string>()),
                                                    vm["bpe_model"].as<std::string>(),
                                                    vm["case_feature"].as<bool>(),
                                                    vm["joiner_annotate"].as<bool>(),
                                                    vm["joiner_new"].as<bool>(),
                                                    vm["joiner"].as<std::string>(),
                                                    false,
                                                    vm["segment_case"].as<bool>(),
                                                    vm["segment_numbers"].as<bool>());

  std::string line;

  while (std::getline(std::cin, line))
  {
    if (!line.empty())
      std::cout << tokenizer->tokenize(line);

    std::cout << std::endl;
  }

  return 0;
}
