#include <memory>

#include <pybind11/pybind11.h>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>

namespace py = pybind11;

#if PY_MAJOR_VERSION < 3
#  define STR_TYPE py::bytes
#else
#  define STR_TYPE py::str
#endif

template <typename T>
std::vector<T> to_std_vector(const py::list& list)
{
  std::vector<T> vec;
  vec.reserve(list.size());
  for (const auto& item : list)
    vec.emplace_back(item.cast<T>());
  return vec;
}

template <typename T>
py::list to_py_list(const std::vector<T>& vec)
{
  py::list list;
  for (const auto& elem : vec)
    list.append(elem);
  return list;
}

template<>
py::list to_py_list(const std::vector<std::string>& vec)
{
  py::list list;
  for (const auto& elem : vec)
    list.append(STR_TYPE(elem));
  return list;
}

template <typename T>
T copy(const T& v)
{
  return v;
}

template <typename T>
T deepcopy(const T& v, const py::object& dict)
{
  return v;
}

static onmt::SubwordEncoder* make_subword_encoder(const std::string& bpe_model_path,
                                                  const std::string& joiner,
                                                  const std::string& sp_model_path,
                                                  int sp_nbest_size,
                                                  float sp_alpha,
                                                  const std::string& vocabulary_path,
                                                  int vocabulary_threshold)
{
  onmt::SubwordEncoder* encoder = nullptr;

  if (!sp_model_path.empty())
    encoder = new onmt::SentencePiece(sp_model_path, sp_nbest_size, sp_alpha);
  else if (!bpe_model_path.empty())
    encoder = new onmt::BPE(bpe_model_path, joiner);

  if (encoder && !vocabulary_path.empty())
    encoder->load_vocabulary(vocabulary_path, vocabulary_threshold);

  return encoder;
}

static onmt::Tokenizer* make_tokenizer(const std::string& mode,
                                       const onmt::SubwordEncoder* subword_encoder,
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
                                       py::list segment_alphabet) {
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

  onmt::Tokenizer::Mode tok_mode = onmt::Tokenizer::mapMode.at(mode);
  auto tokenizer = new onmt::Tokenizer(tok_mode, subword_encoder, flags, joiner);

  for (const auto& alphabet : segment_alphabet)
    tokenizer->add_alphabet_to_segment(alphabet.cast<std::string>());

  return tokenizer;
}

class TokenizerWrapper
{
public:
  TokenizerWrapper(const TokenizerWrapper& other)
    : _subword_encoder(other._subword_encoder)
    , _tokenizer(other._tokenizer)
  {
  }

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
  : _subword_encoder(make_subword_encoder(
                       bpe_model_path,
                       joiner,
                       sp_model_path,
                       sp_nbest_size,
                       sp_alpha,
                       vocabulary_path.empty() ? bpe_vocab_path : vocabulary_path,
                       vocabulary_path.empty() ? bpe_vocab_threshold : vocabulary_threshold))
  , _tokenizer(make_tokenizer(mode,
                              _subword_encoder.get(),
                              joiner,
                              joiner_annotate,
                              joiner_new,
                              spacer_annotate,
                              spacer_new,
                              case_feature,
                              case_markup,
                              no_substitution,
                              preserve_placeholders,
                              preserve_segmented_tokens,
                              segment_case,
                              segment_numbers,
                              segment_alphabet_change,
                              segment_alphabet))

  {
  }

  py::tuple tokenize(const std::string& text) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    _tokenizer->tokenize(text, words, features);

    py::list words_list = to_py_list(words);

    if (features.empty())
      return py::make_tuple(words_list, py::none());
    else
    {
      std::vector<py::list> features_tmp;
      features_tmp.reserve(features.size());
      for (const auto& feature : features)
        features_tmp.emplace_back(to_py_list(feature));

      return py::make_tuple(words_list, to_py_list(features_tmp));
    }
  }

  py::tuple detokenize_with_ranges(const py::list& words, bool merge_ranges) const
  {
    onmt::Ranges ranges;
    std::string text = _tokenizer->detokenize(to_std_vector<std::string>(words),
                                              ranges, merge_ranges);
    py::list ranges_py;
    for (const auto& pair : ranges)
    {
      auto range = py::make_tuple(pair.second.first, pair.second.second);
      ranges_py.append(py::make_tuple(pair.first, range));
    }

    return py::make_tuple(STR_TYPE(text), py::dict(ranges_py));
  }

  STR_TYPE detokenize(const py::list& words, const py::object& features) const
  {
    std::vector<std::string> words_vec = to_std_vector<std::string>(words);
    std::vector<std::vector<std::string> > features_vec;

    if (!features.is(py::none()))
    {
      for (const auto& list : features)
        features_vec.emplace_back(to_std_vector<std::string>(list.cast<py::list>()));
    }

    return _tokenizer->detokenize(words_vec, features_vec);
  }

private:
  const std::shared_ptr<onmt::SubwordEncoder> _subword_encoder;
  const std::shared_ptr<onmt::Tokenizer> _tokenizer;
};

PYBIND11_MODULE(pyonmttok, m)
{
  py::class_<TokenizerWrapper>(m, "Tokenizer")
    .def(py::init<std::string, std::string, std::string, int, std::string, int, std::string, int, float, std::string, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, py::list>(),
         py::arg("mode"),
         py::arg("bpe_model_path")="",
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
         py::arg("segment_alphabet")=py::list())
    .def("tokenize", &TokenizerWrapper::tokenize, py::arg("text"))
    .def("detokenize", &TokenizerWrapper::detokenize,
         py::arg("tokens"), py::arg("features")=py::none())
    .def("detokenize_with_ranges", &TokenizerWrapper::detokenize_with_ranges,
         py::arg("tokens"), py::arg("merge_ranges")=false)
    .def("__copy__", copy<TokenizerWrapper>)
    .def("__deepcopy__", deepcopy<TokenizerWrapper>)
    ;
}
