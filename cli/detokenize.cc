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
    ("spacer_annotate", po::bool_switch()->default_value(false), "detokenize on spacers")
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
  if (vm["spacer_annotate"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SpacerAnnotate;

  onmt::Tokenizer tokenizer(onmt::Tokenizer::Mode::Conservative,
                            flags,
                            "",
                            vm["joiner"].as<std::string>());

  tokenizer.detokenize_stream(std::cin, std::cout);
  return 0;
}
