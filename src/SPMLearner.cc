/*
*/

#include "onmt/SPMLearner.h"
#include <fstream>
#include "onmt/Tokenizer.h"

#include "flags.h"
#include "sentencepiece_model.pb.h"
#include "sentencepiece_trainer.h"
#include "util.h"

using sentencepiece::NormalizerSpec;
using sentencepiece::TrainerSpec;

namespace {
  static sentencepiece::TrainerSpec kDefaultTrainerSpec;
  static sentencepiece::NormalizerSpec kDefaultNormalizerSpec;
}  // namespace

DEFINE_string(input, "", "comma separated list of input sentences");
DEFINE_string(input_format, kDefaultTrainerSpec.input_format(),
  "Input format. Supported format is `text` or `tsv`.");
DEFINE_string(model_prefix, "", "output model prefix");
DEFINE_string(model_type, "unigram",
  "model algorithm: unigram, bpe, word or char");
DEFINE_int32(vocab_size, kDefaultTrainerSpec.vocab_size(), "vocabulary size");
DEFINE_string(accept_language, "",
  "comma-separated list of languages this model can accept");
DEFINE_double(character_coverage, kDefaultTrainerSpec.character_coverage(),
  "character coverage to determine the minimum symbols");
DEFINE_int32(input_sentence_size, kDefaultTrainerSpec.input_sentence_size(),
  "maximum size of sentences the trainer loads");
DEFINE_int32(mining_sentence_size, kDefaultTrainerSpec.mining_sentence_size(),
  "maximum size of sentences to make seed sentence piece");
DEFINE_int32(training_sentence_size,
  kDefaultTrainerSpec.training_sentence_size(),
  "maximum size of sentences to train sentence pieces");
DEFINE_int32(seed_sentencepiece_size,
  kDefaultTrainerSpec.seed_sentencepiece_size(),
  "the size of seed sentencepieces");
DEFINE_double(shrinking_factor, kDefaultTrainerSpec.shrinking_factor(),
  "Keeps top shrinking_factor pieces with respect to the loss");
DEFINE_int32(num_threads, kDefaultTrainerSpec.num_threads(),
  "number of threads for training");
DEFINE_int32(num_sub_iterations, kDefaultTrainerSpec.num_sub_iterations(),
  "number of EM sub-iterations");
DEFINE_int32(max_sentencepiece_length,
  kDefaultTrainerSpec.max_sentencepiece_length(),
  "maximum length of sentence piece");
DEFINE_bool(split_by_unicode_script,
  kDefaultTrainerSpec.split_by_unicode_script(),
  "use Unicode script to split sentence pieces");
DEFINE_bool(split_by_whitespace, kDefaultTrainerSpec.split_by_whitespace(),
  "use a white space to split sentence pieces");
DEFINE_string(control_symbols, "", "comma separated list of control symbols");
DEFINE_string(user_defined_symbols, "",
  "comma separated list of user defined symbols");
DEFINE_string(normalization_rule_name, "nmt_nfkc",
  "Normalization rule name. "
  "Choose from nfkc or identity");
DEFINE_string(normalization_rule_tsv, "", "Normalization rule TSV file. ");
DEFINE_bool(add_dummy_prefix, kDefaultNormalizerSpec.add_dummy_prefix(),
  "Add dummy whitespace at the beginning of text");
DEFINE_bool(remove_extra_whitespaces,
  kDefaultNormalizerSpec.remove_extra_whitespaces(),
  "Removes leading, trailing, and "
  "duplicate internal whitespace");
DEFINE_bool(hard_vocab_limit, kDefaultTrainerSpec.hard_vocab_limit(),
  "If set to false, --vocab_size is considered as a soft limit.");
DEFINE_int32(unk_id, kDefaultTrainerSpec.unk_id(), "Override UNK (<unk>) id.");
DEFINE_int32(bos_id, kDefaultTrainerSpec.bos_id(),
  "Override BOS (<s>) id. Set -1 to disable BOS.");
DEFINE_int32(eos_id, kDefaultTrainerSpec.eos_id(),
  "Override EOS (</s>) id. Set -1 to disable EOS.");
DEFINE_int32(pad_id, kDefaultTrainerSpec.pad_id(),
  "Override PAD (<pad>) id. Set -1 to disable PAD.");
DEFINE_string(unk_surface, kDefaultTrainerSpec.unk_surface(),
  "Dummy surface string for <unk>. In decoding <unk> is decoded to "
  "`unk_surface`.");
 
namespace onmt
{
  SPMLearner::SPMLearner(bool verbose, Tokenizer *pTokenizer,
                        int argc, char **argv):
              SubwordLearner(verbose, pTokenizer), 
              _argc(argc), _argv(argv),
              _trainer(nullptr) {
  }

  void SPMLearner::ingest(std::istream &is) {
     sentencepiece::flags::ParseCommandLineFlags(_argc, _argv);
     sentencepiece::TrainerSpec trainer_spec;
     sentencepiece::NormalizerSpec normalizer_spec;

     CHECK_OR_HELP(input);
     CHECK_OR_HELP(model_prefix);

     // Populates the value from flags to spec.
#define SetTrainerSpecFromFlag(name) trainer_spec.set_##name(FLAGS_##name);

#define SetNormalizerSpecFromFlag(name) \
  normalizer_spec.set_##name(FLAGS_##name);

#define SetRepeatedTrainerSpecFromFlag(name)                     \
  if (!FLAGS_##name.empty()) {                                   \
    for (const auto v :                                          \
         sentencepiece::string_util::Split(FLAGS_##name, ",")) { \
      trainer_spec.add_##name(v);                                \
    }                                                            \
  }

     SetTrainerSpecFromFlag(input_format);
     SetTrainerSpecFromFlag(model_prefix);
     SetTrainerSpecFromFlag(vocab_size);
     SetTrainerSpecFromFlag(character_coverage);
     SetTrainerSpecFromFlag(input_sentence_size);
     SetTrainerSpecFromFlag(mining_sentence_size);
     SetTrainerSpecFromFlag(training_sentence_size);
     SetTrainerSpecFromFlag(seed_sentencepiece_size);
     SetTrainerSpecFromFlag(shrinking_factor);
     SetTrainerSpecFromFlag(num_threads);
     SetTrainerSpecFromFlag(num_sub_iterations);
     SetTrainerSpecFromFlag(max_sentencepiece_length);
     SetTrainerSpecFromFlag(split_by_unicode_script);
     SetTrainerSpecFromFlag(split_by_whitespace);
     SetTrainerSpecFromFlag(hard_vocab_limit);
     SetTrainerSpecFromFlag(unk_id);
     SetTrainerSpecFromFlag(bos_id);
     SetTrainerSpecFromFlag(eos_id);
     SetTrainerSpecFromFlag(pad_id);
     SetTrainerSpecFromFlag(unk_surface);
     SetRepeatedTrainerSpecFromFlag(input);
     SetRepeatedTrainerSpecFromFlag(accept_language);
     SetRepeatedTrainerSpecFromFlag(control_symbols);
     SetRepeatedTrainerSpecFromFlag(user_defined_symbols);

     normalizer_spec.set_name(FLAGS_normalization_rule_name);
     SetNormalizerSpecFromFlag(normalization_rule_tsv);
     SetNormalizerSpecFromFlag(add_dummy_prefix);
     SetNormalizerSpecFromFlag(remove_extra_whitespaces);

     const std::map<std::string, TrainerSpec::ModelType> kModelTypeMap = {
       { "unigram", TrainerSpec::UNIGRAM },
       { "bpe", TrainerSpec::BPE },
       { "word", TrainerSpec::WORD },
       { "char", TrainerSpec::CHAR } };

     trainer_spec.set_model_type(sentencepiece::port::FindOrDie(
       kModelTypeMap, sentencepiece::string_util::ToLower(FLAGS_model_type)));

     CHECK_OK(sentencepiece::SentencePieceTrainer::Train_init(trainer_spec,
       normalizer_spec, _trainer)); 
       
     _trainer->LoadSentences(&is);
   }

   void SPMLearner::learn(std::ostream &os) {
     _trainer->Train(&os);
   }
}
