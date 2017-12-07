#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <onmt/Tokenizer.h>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::options_description desc("Detokenization");
  desc.add_options()
    ("help", "display available options")
    ("joiner", po::value<std::string>()->default_value(onmt::Tokenizer::joiner_marker), "character used to annotate joiners")
    ("case_feature", po::bool_switch()->default_value(false), "first feature is the case")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cerr << desc << std::endl;
    return 1;
  }

  int flags = 0;
  if (vm["case_feature"].as<bool>())
    flags |= onmt::Tokenizer::Flags::CaseFeature;

  onmt::ITokenizer* tokenizer = new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative,
                                                    flags,
                                                    "",
                                                    vm["joiner"].as<std::string>());

  std::string line;

  while (std::getline(std::cin, line))
  {
    if (!line.empty())
      std::cout << tokenizer->detokenize(line);

    std::cout << std::endl;
  }

  return 0;
}
