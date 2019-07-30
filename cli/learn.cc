#include <iostream>
#include <fstream>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <onmt/Tokenizer.h>
#include <onmt/BPELearner.h>
#ifdef WITH_SP_TRAIN
#  include <onmt/SPMLearner.h>
#endif

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::options_description desc_global("Global Options");
  desc_global.add_options()
    ("help,h", "display available options")
    ("verbose,v", po::bool_switch()->default_value(false), "Turn on debug output");

  po::options_description desc_token("Tokenization Options");
  desc_token.add_options()
    ("mode,m", po::value<std::string>()->default_value("space"), "Define how aggressive should the tokenization be: 'aggressive' only keeps sequences of letters/numbers, 'conservative' allows mix of alphanumeric as in: '2,000', 'E65', 'soft-landing'")
    ("preserve_placeholders", po::bool_switch()->default_value(false), "do not mark placeholders with joiners or spacers")
    ("preserve_segmented_tokens", po::bool_switch()->default_value(false), "do not mark segmented tokens (segment_* options) with joiners or spacers")
    ("case_feature,c", po::bool_switch()->default_value(false), "lowercase corpus and generate case feature")
    ("segment_case", po::bool_switch()->default_value(false), "Segment case feature, splits AbC to Ab C to be able to restore case")
    ("segment_numbers", po::bool_switch()->default_value(false), "Segment numbers into single digits")
    ("segment_alphabet", po::value<std::string>()->default_value(""), "comma-separated list of alphabets on which to segment all letters.")
    ("segment_alphabet_change", po::bool_switch()->default_value(false), "Segment if the alphabet changes between 2 letters.");

  po::options_description desc_subword("Learning Options");
  desc_subword.add_options()
      ("input,i", po::value<std::vector<std::string> >()->multitoken(),
                                             "input files (default standard input)")
      ("output,o", po::value<std::string>(), "output file (default standard output)")
      ("tmpfile", po::value<std::string>()->default_value("input.tmp"),
       "Temporary file for file-based learner")
      ("subword", po::value<std::string>(), "subword")
      ("subargs", po::value<std::vector<std::string> >(), "Arguments for subword learner");

  po::positional_options_description pos;
  pos.add("subargs", -1);

  po::variables_map vm;

  po::parsed_options parsed = po::command_line_parser(argc, argv).
                                options(desc_global.add(desc_token).add(desc_subword)).
                                positional(pos).
                                allow_unregistered().
                                run();

  po::store(parsed, vm);

  bool has_subword = vm.count("subword");

  if (!has_subword && vm.count("help")) {
    std::cout << desc_global << std::endl;
    return 1;
  }

  if (!has_subword) {
    std::cerr << "ERROR: missing `subword`" << std::endl;
    return 0;
  }

  int flags = 0;
  if (vm["case_feature"].as<bool>())
    flags |= onmt::Tokenizer::Flags::CaseFeature;
  if (vm["preserve_placeholders"].as<bool>())
    flags |= onmt::Tokenizer::Flags::PreservePlaceholders;
  if (vm["preserve_segmented_tokens"].as<bool>())
    flags |= onmt::Tokenizer::Flags::PreserveSegmentedTokens;
  if (vm["segment_case"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SegmentCase;
  if (vm["segment_numbers"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SegmentNumbers;
  if (vm["segment_alphabet_change"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SegmentAlphabetChange;

  std::vector<std::string> alphabets_to_segment;
  boost::split(alphabets_to_segment,
               vm["segment_alphabet"].as<std::string>(),
               boost::is_any_of(","));

  onmt::Tokenizer tokenizer(onmt::Tokenizer::str_to_mode(vm["mode"].as<std::string>()), flags);

  std::unordered_map<std::string, std::string> subword_options;
  onmt::SubwordLearner *learner;

  std::string subword = vm["subword"].as<std::string>();
  if (subword == "bpe")
  {
    // ls command has the following options:
    po::options_description bpe_desc("bpe options");
    bpe_desc.add_options()
      ("symbols,s", po::value<int>()->default_value(10000),
                    "Create this many new symbols (each representing a character n-gram)")
      ("min-frequency", po::value<int>()->default_value(2),
                    "Stop if no symbol pair has frequency >= FREQ")
      ("dict-input", po::bool_switch()->default_value(false),
                    "If set, input file is interpreted as a dictionary where each line"
                    "contains a word-count pair")
      ("total-symbols,t", po::bool_switch()->default_value(false),
                    "subtract number of characters from the symbols to be generated"
                    " (so that '--symbols' becomes an estimate for the total number "
                    "of symbols needed to encode text).");

    std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);

    // Parse again...
    try {
      po::store(po::command_line_parser(opts).options(bpe_desc).run(), vm);
    } catch (po::error &e) {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 0;
    }

    if (vm.count("help")) {
      std::cout << bpe_desc << std::endl;
      return 1;
    }

    learner = new onmt::BPELearner(vm["verbose"].as<bool>(),
                                   vm["symbols"].as<int>(),
                                   vm["min-frequency"].as<int>(),
                                   vm["dict-input"].as<bool>(),
                                   vm["total-symbols"].as<bool>());

  }
#ifdef WITH_SP_TRAIN
  else if (subword == "sp") {
    std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
    learner = new onmt::SPMLearner(vm["verbose"].as<bool>(), opts, vm["tmpfile"].as<std::string>());
  }
#endif
  else {
    std::cerr << "ERROR: unknown subword `" << subword << "`" << std::endl;
    return 0;
  }

  if (vm.count("input")) {
    for(const std::string &inputFileName: vm["input"].as<std::vector<std::string> >()) {
      std::cerr << "Parsing " << inputFileName << "..." << std::endl;
      std::ifstream inputFile;
      inputFile.open(inputFileName.c_str());
      inputFile.seekg(0);
      if (inputFile.fail() || !inputFile.good()) {
        std::cout << "ERROR: cannot open file " << inputFileName << " for reading " << std::endl;
        return 0;
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
      return 0;
    }
    pCout = &outputFile;
  }
  learner->learn(*pCout, "Generated with subword_learn cli");

  return 1;
}
