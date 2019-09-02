#include <fstream>
#include <memory>
#include <sstream>

#include <pybind11/pybind11.h>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>

#include <onmt/BPELearner.h>
#include <onmt/SPMLearner.h>

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

class TokenizerWrapper
{
public:
  TokenizerWrapper(const TokenizerWrapper& other)
    : _tokenizer(other._tokenizer)
  {
  }

  TokenizerWrapper(onmt::Tokenizer* tokenizer)
    : _tokenizer(tokenizer)
  {
  }

  TokenizerWrapper(const std::string& mode,
                   const std::string& bpe_model_path,
                   const std::string& bpe_vocab_path,
                   int bpe_vocab_threshold,
                   std::string vocabulary_path,
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
                   bool support_prior_joiners,
                   py::list segment_alphabet)
  {
    onmt::SubwordEncoder* subword_encoder = nullptr;

    if (!sp_model_path.empty())
      subword_encoder = new onmt::SentencePiece(sp_model_path, sp_nbest_size, sp_alpha);
    else if (!bpe_model_path.empty())
      subword_encoder = new onmt::BPE(bpe_model_path, joiner);

    if (vocabulary_path.empty())
    {
      // Backward compatibility.
      vocabulary_path = bpe_vocab_path;
      vocabulary_threshold = bpe_vocab_threshold;
    }

    if (subword_encoder && !vocabulary_path.empty())
      subword_encoder->load_vocabulary(vocabulary_path, vocabulary_threshold);

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
    if (support_prior_joiners)
      flags |= onmt::Tokenizer::Flags::SupportPriorJoiners;

    auto tokenizer = new onmt::Tokenizer(onmt::Tokenizer::str_to_mode(mode),
                                         subword_encoder,
                                         flags,
                                         joiner);

    for (const auto& alphabet : segment_alphabet)
      tokenizer->add_alphabet_to_segment(alphabet.cast<std::string>());

    _tokenizer.reset(tokenizer);
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

  void tokenize_file(const std::string& input_path,
                     const std::string& output_path,
                     int num_threads)
  {
    py::gil_scoped_release release;
    std::ifstream in(input_path);
    std::ofstream out(output_path);
    _tokenizer->tokenize_stream(in, out, num_threads);
  }

  void detokenize_file(const std::string& input_path,
                       const std::string& output_path)
  {
    py::gil_scoped_release release;
    std::ifstream in(input_path);
    std::ofstream out(output_path);
    _tokenizer->detokenize_stream(in, out);
  }

  const std::shared_ptr<const onmt::Tokenizer> get() const
  {
    return _tokenizer;
  }

private:
  std::shared_ptr<const onmt::Tokenizer> _tokenizer;
};

class SubwordLearnerWrapper
{
public:
  SubwordLearnerWrapper(const TokenizerWrapper* tokenizer, onmt::SubwordLearner* learner)
    : _learner(learner)
  {
    if (tokenizer)
      _tokenizer = tokenizer->get();
  }

  virtual ~SubwordLearnerWrapper() = default;

  void ingest_file(const std::string& path)
  {
    py::gil_scoped_release release;
    std::ifstream in(path);
    _learner->ingest(in, _tokenizer.get());
  }

  void ingest(const std::string& text)
  {
    std::istringstream in(text);
    _learner->ingest(in, _tokenizer.get());
  }

  TokenizerWrapper learn(const std::string& model_path, bool verbose)
  {
    {
      py::gil_scoped_release release;
      std::ofstream out(model_path);
      _learner->learn(out, nullptr, verbose);
    }

    auto new_tokenizer = create_tokenizer(model_path, _tokenizer.get());
    return TokenizerWrapper(new_tokenizer);
  }

protected:
  std::shared_ptr<const onmt::Tokenizer> _tokenizer;
  std::unique_ptr<onmt::SubwordLearner> _learner;

  // Create a new tokenizer with subword encoding configured.
  virtual onmt::Tokenizer* create_tokenizer(const std::string& model_path,
                                            const onmt::Tokenizer* tokenizer) const = 0;
};

class BPELearnerWrapper : public SubwordLearnerWrapper
{
public:
  BPELearnerWrapper(const TokenizerWrapper* tokenizer,
                    int symbols,
                    int min_frequency,
                    bool total_symbols)
    : SubwordLearnerWrapper(tokenizer,
                            new onmt::BPELearner(false,
                                                 symbols,
                                                 min_frequency,
                                                 false,
                                                 total_symbols))
  {
  }

protected:
  onmt::Tokenizer* create_tokenizer(const std::string& model_path,
                                    const onmt::Tokenizer* tokenizer) const
  {
    onmt::Tokenizer* new_tokenizer = nullptr;
    if (!tokenizer)
      new_tokenizer = new onmt::Tokenizer(onmt::Tokenizer::Mode::Space);
    else
      new_tokenizer = new onmt::Tokenizer(*tokenizer);
    new_tokenizer->set_bpe_model(model_path);
    return new_tokenizer;
  }
};

static std::unordered_map<std::string, std::string> parse_kwargs(py::kwargs kwargs)
{
  std::unordered_map<std::string, std::string> map;
  map.reserve(kwargs.size());
  for (auto& item : kwargs)
    map.emplace(item.first.cast<std::string>(), py::str(item.second).cast<std::string>());
  return map;
}

static std::string create_temp_dir()
{
  py::object tempfile = py::module::import("tempfile");
  py::object mkdtemp = tempfile.attr("mkdtemp");
  return mkdtemp().cast<std::string>();
}

class SentencePieceLearnerWrapper : public SubwordLearnerWrapper
{
public:
  SentencePieceLearnerWrapper(const TokenizerWrapper* tokenizer,
                              py::kwargs kwargs)
    : SubwordLearnerWrapper(tokenizer, new onmt::SPMLearner(false, parse_kwargs(kwargs), ""))
    , _tmp_dir(create_temp_dir())
  {
    dynamic_cast<onmt::SPMLearner*>(_learner.get())->set_input_filename(_tmp_dir + "/input.txt");
  }

  ~SentencePieceLearnerWrapper()
  {
    py::object os = py::module::import("os");
    py::object rmdir = os.attr("rmdir");
    rmdir(_tmp_dir);
  }

protected:
  onmt::Tokenizer* create_tokenizer(const std::string& model_path,
                                    const onmt::Tokenizer* tokenizer) const
  {
    if (!tokenizer)
      return new onmt::Tokenizer(model_path);

    auto new_tokenizer = new onmt::Tokenizer(*tokenizer);
    new_tokenizer->set_sp_model(model_path);
    return new_tokenizer;
  }

private:
  std::string _tmp_dir;
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
    .def("tokenize_file", &TokenizerWrapper::tokenize_file,
         py::arg("input_path"),
         py::arg("output_path"),
         py::arg("num_threads")=1)
    .def("detokenize", &TokenizerWrapper::detokenize,
         py::arg("tokens"), py::arg("features")=py::none())
    .def("detokenize_with_ranges", &TokenizerWrapper::detokenize_with_ranges,
         py::arg("tokens"), py::arg("merge_ranges")=false)
    .def("detokenize_file", &TokenizerWrapper::detokenize_file,
         py::arg("input_path"),
         py::arg("output_path"))
    .def("__copy__", copy<TokenizerWrapper>)
    .def("__deepcopy__", deepcopy<TokenizerWrapper>)
    ;

  py::class_<BPELearnerWrapper>(m, "BPELearner")
    .def(py::init<const TokenizerWrapper*, int, int, bool>(),
         py::arg("tokenizer")=py::none(),
         py::arg("symbols")=10000,
         py::arg("min_frequency")=2,
         py::arg("total_symbols")=false)
    .def("ingest", &BPELearnerWrapper::ingest, py::arg("text"))
    .def("ingest_file", &BPELearnerWrapper::ingest_file, py::arg("path"))
    .def("learn", &BPELearnerWrapper::learn,
         py::arg("model_path"), py::arg("verbose")=false)
    ;

  py::class_<SentencePieceLearnerWrapper>(m, "SentencePieceLearner")
    .def(py::init<const TokenizerWrapper*, py::kwargs>(),
         py::arg("tokenizer")=py::none())
    .def("ingest", &SentencePieceLearnerWrapper::ingest, py::arg("text"))
    .def("ingest_file", &SentencePieceLearnerWrapper::ingest_file, py::arg("path"))
    .def("learn", &SentencePieceLearnerWrapper::learn,
         py::arg("model_path"), py::arg("verbose")=false)
    ;
}
