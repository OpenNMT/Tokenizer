#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#ifdef WITH_SP
#  include <onmt/SentencePiece.h>
#endif

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::options_description desc("Tokenization");
  desc.add_options()
    ("help,h", "display available options")
    ("mode,m", po::value<std::string>()->default_value("conservative"), "Define how aggressive should the tokenization be: 'aggressive' only keeps sequences of letters/numbers, 'conservative' allows mix of alphanumeric as in: '2,000', 'E65', 'soft-landing'")
    ("joiner_annotate,j", po::bool_switch()->default_value(false), "mark joints with joiner tokens (mutually exclusive with spacer_annotate)")
    ("joiner", po::value<std::string>()->default_value(onmt::Tokenizer::joiner_marker), "joiner token")
    ("joiner_new", po::bool_switch()->default_value(false), "make joiners independent tokens")
    ("spacer_annotate", po::bool_switch()->default_value(false), "mark spaces with spacer tokens (mutually exclusive with joiner_annotate)")
    ("spacer_new", po::bool_switch()->default_value(false), "make spacers independent tokens")
    ("preserve_placeholders", po::bool_switch()->default_value(false), "do not mark placeholders with joiners or spacers")
    ("preserve_segmented_tokens", po::bool_switch()->default_value(false), "do not mark segmented tokens (segment_* options) with joiners or spacers")
    ("case_feature,c", po::bool_switch()->default_value(false), "lowercase corpus and generate case feature")
    ("case_markup", po::bool_switch()->default_value(false), "lowercase corpus and inject case markup tokens")
    ("segment_case", po::bool_switch()->default_value(false), "Segment case feature, splits AbC to Ab C to be able to restore case")
    ("segment_numbers", po::bool_switch()->default_value(false), "Segment numbers into single digits")
    ("segment_alphabet", po::value<std::string>()->default_value(""), "comma-separated list of alphabets on which to segment all letters.")
    ("segment_alphabet_change", po::bool_switch()->default_value(false), "Segment if the alphabet changes between 2 letters.")
    ("support_prior_joiners", po::bool_switch()->default_value(false), "if text has existing joiner marks, keep them")
    ("bpe_model_path", po::value<std::string>(), "Path to the BPE model")
    ("bpe_model,b", po::value<std::string>()->default_value(""), "Aliases for --bpe_model_path")
    ("bpe_vocab", po::value<std::string>()->default_value(""), "Deprecated, see --vocabulary.")
    ("bpe_vocab_threshold", po::value<int>()->default_value(50), "Depracted, see --vocabulary_threshold.")
    ("vocabulary", po::value<std::string>()->default_value(""), "Vocabulary file. If provided, sentences are encoded to subword present in this vocabulary.")
    ("vocabulary_threshold", po::value<int>()->default_value(0), "Vocabulary threshold. If vocabulary is provided, any word with frequency < threshold will be treated as OOV.")
    ("num_threads", po::value<int>()->default_value(1), "Number of threads to use.")
#ifdef WITH_SP
    ("sp_model_path", po::value<std::string>(), "Path to the SentencePiece model")
    ("sp_model,s", po::value<std::string>()->default_value(""), "Aliases for --sp_model_path")
    ("sp_nbest_size", po::value<int>()->default_value(0), "number of candidates for the SentencePiece sampling API")
    ("sp_alpha", po::value<float>()->default_value(0.1), "smoothing parameter for the SentencePiece sampling API")
#endif
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
  if (vm["case_markup"].as<bool>())
    flags |= onmt::Tokenizer::Flags::CaseMarkup;
  if (vm["joiner_annotate"].as<bool>())
    flags |= onmt::Tokenizer::Flags::JoinerAnnotate;
  if (vm["joiner_new"].as<bool>())
    flags |= onmt::Tokenizer::Flags::JoinerNew;
  if (vm["spacer_annotate"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SpacerAnnotate;
  if (vm["spacer_new"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SpacerNew;
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
  if (vm["support_prior_joiners"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SupportPriorJoiners;

  std::vector<std::string> alphabets_to_segment;
  boost::split(alphabets_to_segment,
               vm["segment_alphabet"].as<std::string>(),
               boost::is_any_of(","));

  std::string vocabulary = vm["vocabulary"].as<std::string>();
  int vocabulary_threshold = vm["vocabulary_threshold"].as<int>();
  if (vocabulary.empty())
  {
    // Backward compatibility with previous option names.
    vocabulary = vm["bpe_vocab"].as<std::string>();
    vocabulary_threshold = vm["bpe_vocab_threshold"].as<int>();
  }

  std::unique_ptr<onmt::SubwordEncoder> subword_encoder;
  std::string bpe_model = (vm.count("bpe_model_path")
                           ? vm["bpe_model_path"].as<std::string>()
                           : vm["bpe_model"].as<std::string>());
  if (!bpe_model.empty())
    subword_encoder.reset(new onmt::BPE(bpe_model, vm["joiner"].as<std::string>()));
#ifdef WITH_SP
  else
  {
    std::string sp_model = (vm.count("sp_model_path")
                            ? vm["sp_model_path"].as<std::string>()
                            : vm["sp_model"].as<std::string>());
    if (!sp_model.empty())
      subword_encoder.reset(new onmt::SentencePiece(sp_model,
                                                    vm["sp_nbest_size"].as<int>(),
                                                    vm["sp_alpha"].as<float>()));
  }
#endif

  if (subword_encoder && !vocabulary.empty())
    subword_encoder->load_vocabulary(vocabulary, vocabulary_threshold);

  onmt::Tokenizer tokenizer(onmt::Tokenizer::str_to_mode(vm["mode"].as<std::string>()),
                            subword_encoder.get(),
                            flags,
                            vm["joiner"].as<std::string>());

  for (const auto& alphabet : alphabets_to_segment)
  {
    if (!alphabet.empty() && !tokenizer.add_alphabet_to_segment(alphabet))
      std::cerr << "WARNING: " << alphabet << " alphabet is not supported" << std::endl;
  }

  tokenizer.tokenize_stream(std::cin, std::cout, vm["num_threads"].as<int>());
  return 0;
}
