#define BOOST_PYTHON_MAX_ARITY 24
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>

namespace py = boost::python;

template <typename T>
static std::vector<T> to_std_vector(const py::object& iterable)
{
  return std::vector<T>(py::stl_input_iterator<T>(iterable),
                        py::stl_input_iterator<T>());
}


template <typename T>
static py::list to_py_list(const std::vector<T> vec)
{
  py::list list;
  for (const auto& elem : vec)
    list.append(elem);
  return list;
}

typedef std::vector<std::string> StringVec;

class TokenizerWrapper
{
public:
  TokenizerWrapper(const std::string& mode,
                   const std::string& bpe_model_path,
                   const std::string& bpe_vocab_path,
                   int bpe_vocab_threshold,
                   const std::string& vocabulary_path,
                   int vocabulary_threshold,
                   const std::string& sp_model_path,
                   int sp_nbest_size,
                   float sp_alpha,
                   const std::string& joiner,
                   bool joiner_annotate,
                   bool joiner_new,
                   bool spacer_annotate,
                   bool spacer_new,
                   bool case_feature,
                   bool case_markup,
                   bool no_substitution,
                   bool preserve_placeholders,
                   bool preserve_segmented_tokens,
                   bool segment_case,
                   bool segment_numbers,
                   bool segment_alphabet_change,
                   py::list segment_alphabet)
  : _subword_encoder(nullptr)
  , _tokenizer(nullptr)
  {
    int flags = 0;
    if (joiner_annotate)
      flags |= onmt::Tokenizer::Flags::JoinerAnnotate;
    if (joiner_new)
      flags |= onmt::Tokenizer::Flags::JoinerNew;
    if (spacer_annotate)
      flags |= onmt::Tokenizer::Flags::SpacerAnnotate;
    if (spacer_new)
      flags |= onmt::Tokenizer::Flags::SpacerNew;
    if (case_feature)
      flags |= onmt::Tokenizer::Flags::CaseFeature;
    if (case_markup)
      flags |= onmt::Tokenizer::Flags::CaseMarkup;
    if (no_substitution)
      flags |= onmt::Tokenizer::Flags::NoSubstitution;
    if (preserve_placeholders)
      flags |= onmt::Tokenizer::Flags::PreservePlaceholders;
    if (preserve_segmented_tokens)
      flags |= onmt::Tokenizer::Flags::PreserveSegmentedTokens;
    if (segment_case)
      flags |= onmt::Tokenizer::Flags::SegmentCase;
    if (segment_numbers)
      flags |= onmt::Tokenizer::Flags::SegmentNumbers;
    if (segment_alphabet_change)
      flags |= onmt::Tokenizer::Flags::SegmentAlphabetChange;

    std::string vocabulary = vocabulary_path;
    if (vocabulary.empty())
    {
      // Backward compatibility.
      vocabulary = bpe_vocab_path;
      vocabulary_threshold = bpe_vocab_threshold;
    }

    onmt::Tokenizer::Mode tok_mode = onmt::Tokenizer::mapMode.at(mode);

    if (!sp_model_path.empty())
      _subword_encoder = new onmt::SentencePiece(sp_model_path, sp_nbest_size, sp_alpha);
    else if (!bpe_model_path.empty())
      _subword_encoder = new onmt::BPE(bpe_model_path, joiner);

    if (_subword_encoder && !vocabulary.empty())
      _subword_encoder->load_vocabulary(vocabulary, vocabulary_threshold);

    _tokenizer = new onmt::Tokenizer(tok_mode, _subword_encoder, flags, joiner);

    for (auto it = py::stl_input_iterator<std::string>(segment_alphabet);
         it != py::stl_input_iterator<std::string>(); it++)
      _tokenizer->add_alphabet_to_segment(*it);
  }

  ~TokenizerWrapper()
  {
    delete _tokenizer;
    delete _subword_encoder;
  }

  py::tuple tokenize(const std::string& text)
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    _tokenizer->tokenize(text, words, features);

    py::list words_list = to_py_list(words);

    if (features.empty())
      return py::make_tuple(words_list, py::object());
    else
    {
      std::vector<py::list> features_tmp;
      for (const auto& feature : features)
        features_tmp.push_back(to_py_list(feature));

      return py::make_tuple(words_list, to_py_list(features_tmp));
    }
  }

  std::string detokenize(const py::object& words, const py::object& features)
  {
    std::vector<std::string> words_vec = to_std_vector<std::string>(words);
    std::vector<std::vector<std::string> > features_vec;

    if (features != py::object())
    {
      for (auto it = py::stl_input_iterator<py::list>(features);
           it != py::stl_input_iterator<py::list>(); it++)
        features_vec.push_back(to_std_vector<std::string>(*it));
    }

    return _tokenizer->detokenize(words_vec, features_vec);
  }

private:
  onmt::SubwordEncoder* _subword_encoder;
  onmt::Tokenizer* _tokenizer;
};

BOOST_PYTHON_MODULE(tokenizer)
{
  py::class_<TokenizerWrapper>(
      "Tokenizer",
      py::init<std::string, std::string, std::string, int, std::string, int, std::string, int, float, std::string, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, py::list>(
        (py::arg("bpe_model_path")="",
         py::arg("bpe_vocab_path")="",  // Keep for backward compatibility.
         py::arg("bpe_vocab_threshold")=50,  // Keep for backward compatibility.
         py::arg("vocabulary_path")="",
         py::arg("vocabulary_threshold")=0,
         py::arg("sp_model_path")="",
         py::arg("sp_nbest_size")=0,
         py::arg("sp_alpha")=0.1,
         py::arg("joiner")=onmt::Tokenizer::joiner_marker,
         py::arg("joiner_annotate")=false,
         py::arg("joiner_new")=false,
         py::arg("spacer_annotate")=false,
         py::arg("spacer_new")=false,
         py::arg("case_feature")=false,
         py::arg("case_markup")=false,
         py::arg("no_substitution")=false,
         py::arg("preserve_placeholders")=false,
         py::arg("preserve_segmented_tokens")=false,
         py::arg("segment_case")=false,
         py::arg("segment_numbers")=false,
         py::arg("segment_alphabet_change")=false,
         py::arg("segment_alphabet")=py::list())))
    .def("tokenize", &TokenizerWrapper::tokenize)
    .def("detokenize", &TokenizerWrapper::detokenize, (py::arg("features")=py::object()))
    ;
}
