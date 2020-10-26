#include <iostream>
#include <fstream>
#include <unordered_map>

#include <cxxopts.hpp>

#include <onmt/Tokenizer.h>
#include <onmt/BPELearner.h>
#include <onmt/SentencePieceLearner.h>

#include "tokenization_args.h"

int main(int argc, char* argv[])
{
  cxxopts::Options cmd_options("subword_learn",
                               "Subword learning script.\n\nsubword_learn "
                               "[TOKENIZATION_OPTIONS...] -- [SUBWORD_TYPE] "
                               "[SUBWORD_OPTIONS...] < input_file > output_file\n");
  cmd_options.add_options()
    ("h,help", "Show this help")
    ("v,verbose", "Enable verbose output",
     cxxopts::value<bool>()->default_value("false"))
    ;

  add_tokenization_options(cmd_options);

  cmd_options.add_options("Subword learning")
    ("i,input", "Input files (if not set, the standard input is used)",
     cxxopts::value<std::vector<std::string>>())
    ("o,output", "Output file (if not set, the standard output is used)",
     cxxopts::value<std::string>())
    ("tmpfile", "Temporary file for file-based learner",
     cxxopts::value<std::string>()->default_value("input.tmp"))
    ("subword", "Arguments for subword learner",
     cxxopts::value<std::vector<std::string>>());

  cmd_options.parse_positional("subword");

  auto vm = cmd_options.parse(argc, argv);

  if (vm.count("help"))
  {
    std::cout << cmd_options.help() << std::endl;
    return 0;
  }

  onmt::Tokenizer tokenizer(build_tokenization_options(vm));
  onmt::SubwordLearner *learner;

  const auto subword_args = vm["subword"].as<std::vector<std::string>>();
  const auto& subword = subword_args[0];
  if (subword == "bpe")
  {
    cxxopts::Options bpe_options("subword_learn [OPTION...] -- bpe");
    bpe_options.add_options()
      ("h,help", "Show this help")
      ("s,symbols", "Create this many new symbols (each representing a character n-gram)",
       cxxopts::value<int>()->default_value("10000"))
      ("min-frequency", "Stop if no symbol pair has frequency >= FREQ",
       cxxopts::value<int>()->default_value("2"))
      ("dict-input",
       "If set, input file is interpreted as a dictionary where each line "
       "contains a word-count pair",
       cxxopts::value<bool>()->default_value("false"))
      ("t,total-symbols",
       "Subtract number of characters from the symbols to be generated "
       "(so that '--symbols' becomes an estimate for the total number of "
       "symbols needed to encode text)",
       cxxopts::value<bool>()->default_value("false"))
      ;

    // Parse BPE options only.
    int subargc = subword_args.size();
    char** subargv = new char*[subargc];
    for (int i = 0; i < subargc; ++i)
      subargv[i] = const_cast<char*>(subword_args[i].c_str());
    auto subvm = bpe_options.parse(subargc, subargv);
    delete [] subargv;

    if (subvm.count("help"))
    {
      std::cout << bpe_options.help() << std::endl;
      return 0;
    }

    learner = new onmt::BPELearner(vm["verbose"].as<bool>(),
                                   subvm["symbols"].as<int>(),
                                   subvm["min-frequency"].as<int>(),
                                   subvm["dict-input"].as<bool>(),
                                   subvm["total-symbols"].as<bool>());

  }
  else if (subword == "sentencepiece") {
    learner = new onmt::SentencePieceLearner(vm["verbose"].as<bool>(),
                                             std::vector<std::string>(subword_args.begin() + 1,
                                                                      subword_args.end()),
                                             vm["tmpfile"].as<std::string>());
    return 1;
  }
  else {
    std::cerr << "ERROR: invalid subword type: " << subword << " (accepted: bpe, sentencepiece)" << std::endl;
    return 1;
  }

  if (vm.count("input")) {
    for(const std::string &inputFileName: vm["input"].as<std::vector<std::string> >()) {
      std::cerr << "Parsing " << inputFileName << "..." << std::endl;
      std::ifstream inputFile;
      inputFile.open(inputFileName.c_str());
      inputFile.seekg(0);
      if (inputFile.fail() || !inputFile.good()) {
        std::cout << "ERROR: cannot open file " << inputFileName << " for reading " << std::endl;
        return 1;
      }
      learner->ingest(inputFile, &tokenizer);
    }
  } else
    learner->ingest(std::cin, &tokenizer);

  std::ostream* pCout = &std::cout;
  std::ofstream outputFile;
  if (vm.count("output")) {
    std::string outputFileName = vm["output"].as<std::string>();
    outputFile.open(outputFileName.c_str());
    if (outputFile.fail()) {
      std::cout << "ERROR: cannot open file " << outputFileName << " for writing " << std::endl;
      return 1;
    }
    pCout = &outputFile;
  }
  learner->learn(*pCout, "Generated with subword_learn cli");

  return 0;
}
