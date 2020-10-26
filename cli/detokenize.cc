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
    std::cout << cmd_options.help() << std::endl;
    return 0;
  }

  onmt::Tokenizer::Options options;
  options.case_feature = vm["case_feature"].as<bool>();
  options.spacer_annotate = vm["spacer_annotate"].as<bool>();
  options.joiner = vm["joiner"].as<std::string>();
  onmt::Tokenizer tokenizer(std::move(options));

  tokenizer.detokenize_stream(std::cin, std::cout);
  return 0;
}
