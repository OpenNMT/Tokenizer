#include <iostream>

#include <cxxopts.hpp>

#include <onmt/Tokenizer.h>

int main(int argc, char* argv[])
{
  cxxopts::Options cmd_options("detokenize");
  cmd_options.add_options()
    ("h,help", "Show this help")
    ("joiner", "Set the joiner token",
     cxxopts::value<std::string>()->default_value(onmt::Tokenizer::joiner_marker))
    ("spacer_annotate", "Run spacer detokenization instead of joiner detokenization",
     cxxopts::value<bool>()->default_value("false"))
    ("case_feature", "Apply the generated case feature",
     cxxopts::value<bool>()->default_value("false"))
    ;

  auto vm = cmd_options.parse(argc, argv);

  if (vm.count("help"))
  {
    std::cerr << cmd_options.help() << std::endl;
    return 0;
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
