#include <fstream>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <unicode/unistr.h>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>
#include <onmt/BPELearner.h>
#include <onmt/SentencePieceLearner.h>

namespace py = pybind11;
using namespace pybind11::literals;

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
  py::list list(vec.size());
  for (size_t i = 0; i < vec.size(); ++i)
    list[i] = vec[i];
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


static py::tuple
build_tokenization_result(const std::vector<std::string>& words,
                          const std::vector<std::vector<std::string>>& features)
{
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

class TokenizerWrapper
{
public:
  TokenizerWrapper(const TokenizerWrapper& other)
    : _tokenizer(other._tokenizer)
  {
  }

  TokenizerWrapper(const onmt::Tokenizer* tokenizer)
    : _tokenizer(tokenizer)
  {
  }

  TokenizerWrapper(const std::string& mode,
                   const std::string& bpe_model_path,
                   const std::string& bpe_vocab_path,
                   int bpe_vocab_threshold,
                   float bpe_dropout,
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
                   bool soft_case_regions,
                   bool no_substitution,
                   bool preserve_placeholders,
                   bool preserve_segmented_tokens,
                   bool segment_case,
                   bool segment_numbers,
                   bool segment_alphabet_change,
                   bool support_prior_joiners,
                   const py::object& segment_alphabet)
  {
    onmt::SubwordEncoder* subword_encoder = nullptr;

    if (!sp_model_path.empty())
      subword_encoder = new onmt::SentencePiece(sp_model_path, sp_nbest_size, sp_alpha);
    else if (!bpe_model_path.empty())
      subword_encoder = new onmt::BPE(bpe_model_path, bpe_dropout);

    if (vocabulary_path.empty())
    {
      // Backward compatibility.
      vocabulary_path = bpe_vocab_path;
      vocabulary_threshold = bpe_vocab_threshold;
    }

    onmt::Tokenizer::Options options;
    options.mode = onmt::Tokenizer::str_to_mode(mode);
    options.no_substitution = no_substitution;
    options.case_feature = case_feature;
    options.case_markup = case_markup;
    options.soft_case_regions = soft_case_regions;
    options.joiner_annotate = joiner_annotate;
    options.joiner_new = joiner_new;
    options.joiner = joiner;
    options.spacer_annotate = spacer_annotate;
    options.spacer_new = spacer_new;
    options.preserve_placeholders = preserve_placeholders;
    options.preserve_segmented_tokens = preserve_segmented_tokens;
    options.support_prior_joiners = support_prior_joiners;
    options.segment_case = segment_case;
    options.segment_numbers = segment_numbers;
    options.segment_alphabet_change = segment_alphabet_change;
    if (!segment_alphabet.is(py::none()))
      options.segment_alphabet = to_std_vector<std::string>(segment_alphabet.cast<py::list>());

    if (subword_encoder && !vocabulary_path.empty())
      subword_encoder->load_vocabulary(vocabulary_path, vocabulary_threshold, &options);

    _tokenizer.reset(new onmt::Tokenizer(options,
                                         std::shared_ptr<const onmt::SubwordEncoder>(subword_encoder)));
  }

  py::dict get_options() const
  {
    const auto& options = _tokenizer->get_options();
    return py::dict(
      "mode"_a=onmt::Tokenizer::mode_to_str(options.mode),
      "no_substitution"_a=options.no_substitution,
      "case_feature"_a=options.case_feature,
      "case_markup"_a=options.case_markup,
      "soft_case_regions"_a=options.soft_case_regions,
      "joiner_annotate"_a=options.joiner_annotate,
      "joiner_new"_a=options.joiner_new,
      "joiner"_a=options.joiner,
      "spacer_annotate"_a=options.spacer_annotate,
      "spacer_new"_a=options.spacer_new,
      "preserve_placeholders"_a=options.preserve_placeholders,
      "preserve_segmented_tokens"_a=options.preserve_segmented_tokens,
      "support_prior_joiners"_a=options.support_prior_joiners,
      "segment_case"_a=options.segment_case,
      "segment_numbers"_a=options.segment_numbers,
      "segment_alphabet_change"_a=options.segment_alphabet_change,
      "segment_alphabet"_a=to_py_list(options.segment_alphabet)
      );
  }

  py::object tokenize(const std::string& text, const bool as_token_objects) const
  {
    if (as_token_objects)
    {
      std::vector<onmt::Token> tokens;
      _tokenizer->tokenize(text, tokens);
      return to_py_list(tokens);
    }

    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    _tokenizer->tokenize(text, words, features);
    return build_tokenization_result(words, features);
  }

  py::tuple serialize_tokens(const py::list& tokens) const
  {
    std::vector<onmt::Token> tokens_vec = to_std_vector<onmt::Token>(tokens);
    std::vector<std::string> words;
    std::vector<std::vector<std::string>> features;
    _tokenizer->finalize_tokens(tokens_vec, words, features);
    return build_tokenization_result(words, features);
  }

  py::list deserialize_tokens(const py::list& words, const py::object& features) const
  {
    std::vector<std::string> words_vec = to_std_vector<std::string>(words);
    std::vector<std::vector<std::string>> features_vec;

    if (!features.is(py::none()))
    {
      for (const auto& list : features)
        features_vec.emplace_back(to_std_vector<std::string>(list.cast<py::list>()));
    }

    std::vector<onmt::Token> tokens;
    _tokenizer->annotate_tokens(words_vec, features_vec, tokens);
    return to_py_list(tokens);
  }

  py::tuple detokenize_with_ranges(const py::list& words,
                                   bool merge_ranges,
                                   bool with_unicode_ranges) const
  {
    onmt::Ranges ranges;
    std::string text;
    if (words.size() > 0)
    {
      if (py::isinstance<onmt::Token>(words[0]))
        text = _tokenizer->detokenize(to_std_vector<onmt::Token>(words),
                                      ranges, merge_ranges);
      else
        text = _tokenizer->detokenize(to_std_vector<std::string>(words),
                                      ranges, merge_ranges);
    }

    if (with_unicode_ranges)
    {
      onmt::Ranges unicode_ranges;
      for (const auto& pair : ranges)
      {
        const size_t word_index = pair.first;
        const onmt::Range& range = pair.second;
        const icu::UnicodeString prefix(text.c_str(), range.first);
        const icu::UnicodeString piece(text.c_str() + range.first, range.second - range.first + 1);
        unicode_ranges.emplace(word_index,
                               onmt::Range(prefix.length(), prefix.length() + piece.length() - 1));
      }
      ranges = std::move(unicode_ranges);
    }

    py::list ranges_py(ranges.size());
    size_t index = 0;
    for (const auto& pair : ranges)
    {
      auto range = py::make_tuple(pair.second.first, pair.second.second);
      ranges_py[index++] = py::make_tuple(pair.first, range);
    }

    return py::make_tuple(text, py::dict(ranges_py));
  }

  std::string detokenize(const py::list& words, const py::object& features) const
  {
    if (words.size() == 0)
      return "";

    if (py::isinstance<onmt::Token>(words[0]))
      return _tokenizer->detokenize(to_std_vector<onmt::Token>(words));

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
                     int num_threads,
                     bool verbose)
  {
    std::ifstream in(input_path);
    if (!in)
      throw std::invalid_argument("Failed to open input file " + input_path);
    std::ofstream out(output_path);
    if (!out)
      throw std::invalid_argument("Failed to open output file " + output_path);
    py::gil_scoped_release release;
    _tokenizer->tokenize_stream(in, out, num_threads, verbose);
  }

  void detokenize_file(const std::string& input_path,
                       const std::string& output_path)
  {
    std::ifstream in(input_path);
    if (!in)
      throw std::invalid_argument("Failed to open input file " + input_path);
    std::ofstream out(output_path);
    if (!out)
      throw std::invalid_argument("Failed to open output file " + output_path);
    py::gil_scoped_release release;
    _tokenizer->detokenize_stream(in, out);
  }

  const std::shared_ptr<const onmt::Tokenizer>& get() const
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
    : _tokenizer(tokenizer ? tokenizer->get() : learner->get_default_tokenizer())
    , _learner(learner)
  {
  }

  virtual ~SubwordLearnerWrapper() = default;

  void ingest_file(const std::string& path)
  {
    std::ifstream in(path);
    if (!in)
      throw std::invalid_argument("Failed to open input file " + path);
    py::gil_scoped_release release;
    _learner->ingest(in, _tokenizer.get());
  }

  void ingest(const std::string& text)
  {
    _learner->ingest(text, _tokenizer.get());
  }

  void ingest_token(const std::string& token)
  {
    _learner->ingest_token(token, _tokenizer.get());
  }

  void ingest_token(const onmt::Token& token)
  {
    _learner->ingest_token(token);
  }

  TokenizerWrapper learn(const std::string& model_path, bool verbose)
  {
    {
      py::gil_scoped_release release;
      _learner->learn(model_path, nullptr, verbose);
    }

    auto* new_subword_encoder = create_subword_encoder(model_path);
    auto* new_tokenizer = new onmt::Tokenizer(*_tokenizer);
    new_tokenizer->set_subword_encoder(std::shared_ptr<const onmt::SubwordEncoder>(new_subword_encoder));
    return TokenizerWrapper(new_tokenizer);
  }

protected:
  virtual onmt::SubwordEncoder* create_subword_encoder(const std::string& model_path) const = 0;

private:
  std::shared_ptr<const onmt::Tokenizer> _tokenizer;
  std::unique_ptr<onmt::SubwordLearner> _learner;
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
  onmt::SubwordEncoder* create_subword_encoder(const std::string& model_path) const override
  {
    return new onmt::BPE(model_path);
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

static std::string create_temp_file()
{
  py::object tempfile = py::module::import("tempfile");
  py::object mkstemp = tempfile.attr("mkstemp");
  py::tuple output = mkstemp();
  auto fd = output[0].cast<int>();
  auto filename = output[1].cast<std::string>();
  close(fd);
  return filename;
}

class SentencePieceLearnerWrapper : public SubwordLearnerWrapper
{
public:
  SentencePieceLearnerWrapper(const TokenizerWrapper* tokenizer,
                              bool keep_vocab,
                              py::kwargs kwargs)
    : SubwordLearnerWrapper(tokenizer,
                            new onmt::SentencePieceLearner(false,
                                                           parse_kwargs(kwargs),
                                                           create_temp_file(),
                                                           keep_vocab))
    , _keep_vocab(keep_vocab)
  {
  }

protected:
  onmt::SubwordEncoder* create_subword_encoder(const std::string& model_path) const override
  {
    std::string sp_model = _keep_vocab ? model_path + ".model" : model_path;
    return new onmt::SentencePiece(sp_model);
  }

private:
  bool _keep_vocab;
};

static onmt::Token create_token(std::string surface,
                                const onmt::TokenType type,
                                const onmt::Casing casing,
                                const bool join_left,
                                const bool join_right,
                                const bool spacer,
                                const bool preserve,
                                const py::object& features) {
  onmt::Token token(std::move(surface));
  token.type = type;
  token.casing = casing;
  token.join_left = join_left;
  token.join_right = join_right;
  token.spacer = spacer;
  token.preserve = preserve;
  if (!features.is(py::none()))
    token.features = to_std_vector<std::string>(features.cast<py::list>());
  return token;
}

static std::string repr_token(const onmt::Token& token) {
  std::string repr = "Token(";
  if (!token.empty())
    repr += "'" + token.surface + "'";
  if (token.type != onmt::TokenType::Word)
    repr += ", type=" + std::string(py::str(py::cast(token.type)));
  if (token.join_left)
    repr += ", join_left=True";
  if (token.join_right)
    repr += ", join_right=True";
  if (token.spacer)
    repr += ", spacer=True";
  if (token.preserve)
    repr += ", preserve=True";
  if (token.has_features())
    repr += ", features=" + std::string(py::repr(to_py_list(token.features)));
  if (token.casing != onmt::Casing::None)
    repr += ", casing=" + std::string(py::str(py::cast(token.casing)));
  repr += ")";
  return repr;
}

static inline py::tuple list_to_tuple(py::list list) {
  py::tuple tuple(list.size());
  for (size_t i = 0; i < list.size(); ++i)
    tuple[i] = list[i];
  return tuple;
}

static ssize_t hash_token(const onmt::Token& token) {
  return py::hash(py::make_tuple(token.surface,
                                 token.type,
                                 token.casing,
                                 token.join_left,
                                 token.join_right,
                                 token.spacer,
                                 token.preserve,
                                 list_to_tuple(py::cast(token.features))));
}

PYBIND11_MODULE(_ext, m)
{
  m.def("is_placeholder", &onmt::Tokenizer::is_placeholder, py::arg("token"));
  m.def("set_random_seed", &onmt::set_random_seed, py::arg("seed"));

  py::enum_<onmt::Casing>(m, "Casing")
    .value("NONE", onmt::Casing::None)
    .value("LOWERCASE", onmt::Casing::Lowercase)
    .value("UPPERCASE", onmt::Casing::Uppercase)
    .value("MIXED", onmt::Casing::Mixed)
    .value("CAPITALIZED", onmt::Casing::Capitalized)
    .export_values();

  py::enum_<onmt::TokenType>(m, "TokenType")
    .value("WORD", onmt::TokenType::Word)
    .value("LEADING_SUBWORD", onmt::TokenType::LeadingSubword)
    .value("TRAILING_SUBWORD", onmt::TokenType::TrailingSubword)
    .export_values();

  py::class_<onmt::Token>(m, "Token")
    .def(py::init<>())
    .def(py::init<std::string>(), py::arg("surface"))
    .def(py::init<const onmt::Token&>(), py::arg("token"))
    .def(py::init(&create_token),
         py::arg("surface"),
         py::arg("type")=onmt::TokenType::Word,
         py::arg("casing")=onmt::Casing::None,
         py::arg("join_left")=false,
         py::arg("join_right")=false,
         py::arg("spacer")=false,
         py::arg("preserve")=false,
         py::arg("features")=py::none())
    .def_readwrite("surface", &onmt::Token::surface)
    .def_readwrite("type", &onmt::Token::type)
    .def_readwrite("join_left", &onmt::Token::join_left)
    .def_readwrite("join_right", &onmt::Token::join_right)
    .def_readwrite("spacer", &onmt::Token::spacer)
    .def_readwrite("preserve", &onmt::Token::preserve)
    .def_readwrite("features", &onmt::Token::features)
    .def_readwrite("casing", &onmt::Token::casing)
    .def("is_placeholder", &onmt::Token::is_placeholder)
    .def("__eq__", &onmt::Token::operator==)
    .def("__hash__", &hash_token)
    .def("__repr__", &repr_token)
    .def(py::pickle(
           [](const onmt::Token& token)
             {
               return py::make_tuple(token.surface,
                                     token.type,
                                     token.casing,
                                     token.join_left,
                                     token.join_right,
                                     token.spacer,
                                     token.preserve,
                                     token.features);
             },
           [](py::tuple t)
             {
               return create_token(t[0].cast<std::string>(),
                                   t[1].cast<onmt::TokenType>(),
                                   t[2].cast<onmt::Casing>(),
                                   t[3].cast<bool>(),
                                   t[4].cast<bool>(),
                                   t[5].cast<bool>(),
                                   t[6].cast<bool>(),
                                   t[7]);
             }
           ));
    ;

  py::class_<TokenizerWrapper>(m, "Tokenizer")
    .def(py::init<const TokenizerWrapper&>(), py::arg("tokenizer"))
    .def(py::init<const std::string&, const std::string&, const std::string&, int, float, std::string, int, const std::string&, int, float, const std::string&, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, bool, const py::object&>(),
         py::arg("mode"),
         py::kw_only(),
         py::arg("bpe_model_path")="",
         py::arg("bpe_vocab_path")="",  // Keep for backward compatibility.
         py::arg("bpe_vocab_threshold")=50,  // Keep for backward compatibility.
         py::arg("bpe_dropout")=0,
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
         py::arg("soft_case_regions")=false,
         py::arg("no_substitution")=false,
         py::arg("preserve_placeholders")=false,
         py::arg("preserve_segmented_tokens")=false,
         py::arg("segment_case")=false,
         py::arg("segment_numbers")=false,
         py::arg("segment_alphabet_change")=false,
         py::arg("support_prior_joiners")=false,
         py::arg("segment_alphabet")=py::none())
    .def_property_readonly("options", &TokenizerWrapper::get_options)
    .def("tokenize", &TokenizerWrapper::tokenize,
         py::arg("text"),
         py::arg("as_token_objects")=false)
    .def("serialize_tokens", &TokenizerWrapper::serialize_tokens,
         py::arg("tokens"))
    .def("deserialize_tokens", &TokenizerWrapper::deserialize_tokens,
         py::arg("tokens"), py::arg("features")=py::none())
    .def("tokenize_file", &TokenizerWrapper::tokenize_file,
         py::arg("input_path"),
         py::arg("output_path"),
         py::arg("num_threads")=1,
         py::arg("verbose")=false)
    .def("detokenize", &TokenizerWrapper::detokenize,
         py::arg("tokens"), py::arg("features")=py::none())
    .def("detokenize_with_ranges", &TokenizerWrapper::detokenize_with_ranges,
         py::arg("tokens"),
         py::arg("merge_ranges")=false,
         py::arg("unicode_ranges")=false)
    .def("detokenize_file", &TokenizerWrapper::detokenize_file,
         py::arg("input_path"),
         py::arg("output_path"))
    .def("__copy__", copy<TokenizerWrapper>)
    .def("__deepcopy__", deepcopy<TokenizerWrapper>)
    ;

  py::class_<SubwordLearnerWrapper>(m, "SubwordLearner")
    .def("ingest", &SubwordLearnerWrapper::ingest, py::arg("text"))
    .def("ingest_file", &SubwordLearnerWrapper::ingest_file, py::arg("path"))
    .def("ingest_token",
         (void (SubwordLearnerWrapper::*)(const std::string&)) &SubwordLearnerWrapper::ingest_token,
         py::arg("token"))
    .def("ingest_token",
         (void (SubwordLearnerWrapper::*)(const onmt::Token&)) &SubwordLearnerWrapper::ingest_token,
         py::arg("token"))
    .def("learn", &SubwordLearnerWrapper::learn,
         py::arg("model_path"), py::arg("verbose")=false)
    ;

  py::class_<BPELearnerWrapper, SubwordLearnerWrapper>(m, "BPELearner")
    .def(py::init<const TokenizerWrapper*, int, int, bool>(),
         py::arg("tokenizer")=py::none(),
         py::arg("symbols")=10000,
         py::arg("min_frequency")=2,
         py::arg("total_symbols")=false)
    ;

  py::class_<SentencePieceLearnerWrapper, SubwordLearnerWrapper>(m, "SentencePieceLearner")
    .def(py::init<const TokenizerWrapper*, bool, py::kwargs>(),
         py::arg("tokenizer")=py::none(),
         py::arg("keep_vocab")=false)
    ;
}
