#include <fstream>
#include <memory>
#include <optional>
#include <variant>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <onmt/Tokenizer.h>
#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>
#include <onmt/BPELearner.h>
#include <onmt/SentencePieceLearner.h>

namespace py = pybind11;
using namespace pybind11::literals;


class TokenizerWrapper
{
public:
  TokenizerWrapper(std::shared_ptr<const onmt::Tokenizer> tokenizer)
    : _tokenizer(std::move(tokenizer))
  {
  }

  TokenizerWrapper(const std::string& mode,
                   const std::optional<std::string>& lang,
                   const std::optional<std::string>& bpe_model_path,
                   const std::optional<std::string>& bpe_vocab_path,
                   int bpe_vocab_threshold,
                   float bpe_dropout,
                   const std::optional<std::string>& vocabulary_path,
                   int vocabulary_threshold,
                   const std::optional<std::string>& sp_model_path,
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
                   bool with_separators,
                   bool preserve_placeholders,
                   bool preserve_segmented_tokens,
                   bool segment_case,
                   bool segment_numbers,
                   bool segment_alphabet_change,
                   bool support_prior_joiners,
                   const std::optional<std::vector<std::string>>& segment_alphabet)
  {
    std::shared_ptr<onmt::SubwordEncoder> subword_encoder;

    if (sp_model_path)
      subword_encoder = std::make_shared<onmt::SentencePiece>(sp_model_path.value(),
                                                              sp_nbest_size,
                                                              sp_alpha);
    else if (bpe_model_path)
      subword_encoder = std::make_shared<onmt::BPE>(bpe_model_path.value(), bpe_dropout);

    onmt::Tokenizer::Options options;
    options.mode = onmt::Tokenizer::str_to_mode(mode);
    options.lang = lang.value_or("");
    options.no_substitution = no_substitution;
    options.with_separators = with_separators;
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
    if (segment_alphabet)
      options.segment_alphabet = segment_alphabet.value();

    if (subword_encoder)
    {
      if (vocabulary_path)
        subword_encoder->load_vocabulary(vocabulary_path.value(), vocabulary_threshold, &options);
      else if (bpe_vocab_path)  // Backward compatibility.
        subword_encoder->load_vocabulary(bpe_vocab_path.value(), bpe_vocab_threshold, &options);
    }

    _tokenizer = std::make_shared<onmt::Tokenizer>(options, subword_encoder);
  }

  virtual ~TokenizerWrapper() = default;

  py::dict get_options() const
  {
    const auto& options = _tokenizer->get_options();
    return py::dict(
      "mode"_a=onmt::Tokenizer::mode_to_str(options.mode),
      "lang"_a=options.lang,
      "no_substitution"_a=options.no_substitution,
      "with_separators"_a=options.with_separators,
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
      "segment_alphabet"_a=options.segment_alphabet
      );
  }

  std::variant<
    std::pair<std::vector<std::string>, std::optional<std::vector<std::vector<std::string>>>>,
    std::vector<onmt::Token>>
  tokenize(const std::string& text,
           const bool as_token_objects,
           const bool training) const
  {
    std::vector<onmt::Token> tokens;
    _tokenizer->tokenize(text, tokens, training);
    if (as_token_objects)
      return std::move(tokens);
    return serialize_tokens(tokens);
  }

  std::pair<std::vector<std::string>, std::optional<std::vector<std::vector<std::string>>>>
  serialize_tokens(const std::vector<onmt::Token>& tokens) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string>> features;
    _tokenizer->finalize_tokens(tokens, words, features);

    std::optional<std::vector<std::vector<std::string>>> optional_features;
    if (!features.empty())
      optional_features = std::move(features);
    return std::make_pair(std::move(words), std::move(optional_features));
  }

  std::vector<onmt::Token>
  deserialize_tokens(const std::vector<std::string>& words,
                     const std::optional<std::vector<std::vector<std::string>>>& features) const
  {
    std::vector<onmt::Token> tokens;
    _tokenizer->annotate_tokens(words,
                                features.value_or(std::vector<std::vector<std::string>>()),
                                tokens);
    return tokens;
  }

  template <typename T>
  std::pair<std::string, onmt::Ranges> detokenize_with_ranges(const std::vector<T>& tokens,
                                                              bool merge_ranges,
                                                              bool with_unicode_ranges) const
  {
    onmt::Ranges ranges;
    std::string text = _tokenizer->detokenize(tokens, ranges, merge_ranges);

    if (with_unicode_ranges)
    {
      onmt::Ranges unicode_ranges;
      for (const auto& pair : ranges)
      {
        const size_t word_index = pair.first;
        const onmt::Range& range = pair.second;
        const std::string prefix(text.c_str(), range.first);
        const std::string piece(text.c_str() + range.first, range.second - range.first + 1);
        const size_t prefix_length = onmt::unicode::utf8len(prefix);
        const size_t piece_length = onmt::unicode::utf8len(piece);
        unicode_ranges.emplace(word_index,
                               onmt::Range(prefix_length, prefix_length + piece_length - 1));
      }
      ranges = std::move(unicode_ranges);
    }

    return std::make_pair(std::move(text), std::move(ranges));
  }

  std::string detokenize(const std::vector<onmt::Token>& tokens) const
  {
    return _tokenizer->detokenize(tokens);
  }

  std::string detokenize(const std::vector<std::string>& tokens,
                         const std::optional<std::vector<std::vector<std::string>>>& features) const
  {
    return _tokenizer->detokenize(tokens, features.value_or(std::vector<std::vector<std::string>>()));
  }

  void tokenize_file(const std::string& input_path,
                     const std::string& output_path,
                     int num_threads,
                     bool verbose,
                     bool training,
                     const std::string& tokens_delimiter)
  {
    std::ifstream in(input_path);
    if (!in)
      throw std::invalid_argument("Failed to open input file " + input_path);
    std::ofstream out(output_path);
    if (!out)
      throw std::invalid_argument("Failed to open output file " + output_path);
    _tokenizer->tokenize_stream(in, out, num_threads, verbose, training, tokens_delimiter);
  }

  void detokenize_file(const std::string& input_path,
                       const std::string& output_path,
                       const std::string& tokens_delimiter)
  {
    std::ifstream in(input_path);
    if (!in)
      throw std::invalid_argument("Failed to open input file " + input_path);
    std::ofstream out(output_path);
    if (!out)
      throw std::invalid_argument("Failed to open output file " + output_path);
    _tokenizer->detokenize_stream(in, out, tokens_delimiter);
  }

  const std::shared_ptr<const onmt::Tokenizer>& get() const
  {
    return _tokenizer;
  }

private:
  std::shared_ptr<const onmt::Tokenizer> _tokenizer;
};

static std::shared_ptr<onmt::Tokenizer>
build_sp_tokenizer(const std::string& model_path,
                   const std::optional<std::string>& vocabulary_path,
                   int vocabulary_threshold,
                   int nbest_size,
                   float alpha)
{
  onmt::Tokenizer::Options options;
  options.mode = onmt::Tokenizer::Mode::None;
  options.no_substitution = true;
  options.spacer_annotate = true;

  auto subword_encoder = std::make_shared<onmt::SentencePiece>(model_path, nbest_size, alpha);
  if (vocabulary_path)
    subword_encoder->load_vocabulary(vocabulary_path.value(), vocabulary_threshold, &options);

  return std::make_shared<onmt::Tokenizer>(options, subword_encoder);
}

class SentencePieceTokenizerWrapper : public TokenizerWrapper
{
public:
  SentencePieceTokenizerWrapper(const std::string& model_path,
                                const std::optional<std::string>& vocabulary_path,
                                int vocabulary_threshold,
                                int nbest_size,
                                float alpha)
    : TokenizerWrapper(build_sp_tokenizer(model_path,
                                          vocabulary_path,
                                          vocabulary_threshold,
                                          nbest_size,
                                          alpha))
  {
  }
};

class SubwordLearnerWrapper
{
public:
  SubwordLearnerWrapper(const std::optional<TokenizerWrapper>& tokenizer,
                        std::unique_ptr<onmt::SubwordLearner> learner)
    : _tokenizer(tokenizer ? tokenizer->get() : learner->get_default_tokenizer())
    , _learner(std::move(learner))
  {
  }

  virtual ~SubwordLearnerWrapper() = default;

  void ingest_file(const std::string& path)
  {
    std::ifstream in(path);
    if (!in)
      throw std::invalid_argument("Failed to open input file " + path);
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
    _learner->learn(model_path, nullptr, verbose);

    auto new_subword_encoder = create_subword_encoder(model_path);
    auto new_tokenizer = std::make_shared<onmt::Tokenizer>(*_tokenizer);
    new_tokenizer->set_subword_encoder(new_subword_encoder);
    return TokenizerWrapper(std::move(new_tokenizer));
  }

protected:
  virtual std::shared_ptr<onmt::SubwordEncoder>
  create_subword_encoder(const std::string& model_path) const = 0;

private:
  std::shared_ptr<const onmt::Tokenizer> _tokenizer;
  std::unique_ptr<onmt::SubwordLearner> _learner;
};

class BPELearnerWrapper : public SubwordLearnerWrapper
{
public:
  BPELearnerWrapper(const std::optional<TokenizerWrapper>& tokenizer,
                    int symbols,
                    int min_frequency,
                    bool total_symbols)
    : SubwordLearnerWrapper(tokenizer,
                            std::make_unique<onmt::BPELearner>(false,
                                                               symbols,
                                                               min_frequency,
                                                               false,
                                                               total_symbols))
  {
  }

protected:
  std::shared_ptr<onmt::SubwordEncoder>
  create_subword_encoder(const std::string& model_path) const override
  {
    return std::make_shared<onmt::BPE>(model_path);
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
  SentencePieceLearnerWrapper(const std::optional<TokenizerWrapper>& tokenizer,
                              bool keep_vocab,
                              py::kwargs kwargs)
    : SubwordLearnerWrapper(tokenizer,
                            std::make_unique<onmt::SentencePieceLearner>(false,
                                                                         parse_kwargs(kwargs),
                                                                         create_temp_file(),
                                                                         keep_vocab))
    , _keep_vocab(keep_vocab)
  {
  }

protected:
  std::shared_ptr<onmt::SubwordEncoder>
  create_subword_encoder(const std::string& model_path) const override
  {
    std::string sp_model = _keep_vocab ? model_path + ".model" : model_path;
    return std::make_shared<onmt::SentencePiece>(sp_model);
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
                                const std::optional<std::vector<std::string>>& features) {
  onmt::Token token(std::move(surface));
  token.type = type;
  token.casing = casing;
  token.join_left = join_left;
  token.join_right = join_right;
  token.spacer = spacer;
  token.preserve = preserve;
  if (features)
    token.features = features.value();
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
    repr += ", features=" + std::string(py::repr(py::cast(token.features)));
  if (token.casing != onmt::Casing::None)
    repr += ", casing=" + std::string(py::str(py::cast(token.casing)));
  repr += ")";
  return repr;
}

static ssize_t hash_token(const onmt::Token& token) {
  return py::hash(py::make_tuple(token.surface,
                                 token.type,
                                 token.casing,
                                 token.join_left,
                                 token.join_right,
                                 token.spacer,
                                 token.preserve,
                                 py::tuple(py::cast(token.features))));
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
    .def("__len__", &onmt::Token::unicode_length)
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
                                   t[7].cast<std::optional<std::vector<std::string>>>());
             }
           ));
    ;

  py::class_<TokenizerWrapper>(m, "Tokenizer")
    .def(py::init<
         const std::string&,
         const std::optional<std::string>&,
         const std::optional<std::string>&,
         const std::optional<std::string>&,
         int,
         float,
         const std::optional<std::string>&,
         int,
         const std::optional<std::string>&,
         int,
         float,
         const std::string&,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         bool,
         const std::optional<std::vector<std::string>>&>(),
         py::arg("mode"),
         py::kw_only(),
         py::arg("lang")=py::none(),
         py::arg("bpe_model_path")=py::none(),
         py::arg("bpe_vocab_path")=py::none(),  // Keep for backward compatibility.
         py::arg("bpe_vocab_threshold")=50,  // Keep for backward compatibility.
         py::arg("bpe_dropout")=0,
         py::arg("vocabulary_path")=py::none(),
         py::arg("vocabulary_threshold")=0,
         py::arg("sp_model_path")=py::none(),
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
         py::arg("with_separators")=false,
         py::arg("preserve_placeholders")=false,
         py::arg("preserve_segmented_tokens")=false,
         py::arg("segment_case")=false,
         py::arg("segment_numbers")=false,
         py::arg("segment_alphabet_change")=false,
         py::arg("support_prior_joiners")=false,
         py::arg("segment_alphabet")=py::none())
    .def(py::init<const TokenizerWrapper&>(), py::arg("tokenizer"))

    .def_property_readonly("options", &TokenizerWrapper::get_options)

    .def("tokenize", &TokenizerWrapper::tokenize,
         py::arg("text"),
         py::arg("as_token_objects")=false,
         py::arg("training")=true,
         py::call_guard<py::gil_scoped_release>())

    .def("detokenize",
         py::overload_cast<
         const std::vector<std::string>&,
         const std::optional<std::vector<std::vector<std::string>>>&>(
           &TokenizerWrapper::detokenize, py::const_),
         py::arg("tokens"),
         py::arg("features")=py::none())
    .def("detokenize",
         py::overload_cast<const std::vector<onmt::Token>&>(
           &TokenizerWrapper::detokenize, py::const_),
         py::arg("tokens"))

    .def("detokenize_with_ranges", &TokenizerWrapper::detokenize_with_ranges<std::string>,
         py::arg("tokens"),
         py::arg("merge_ranges")=false,
         py::arg("unicode_ranges")=false)
    .def("detokenize_with_ranges", &TokenizerWrapper::detokenize_with_ranges<onmt::Token>,
         py::arg("tokens"),
         py::arg("merge_ranges")=false,
         py::arg("unicode_ranges")=false)

    .def("tokenize_file", &TokenizerWrapper::tokenize_file,
         py::arg("input_path"),
         py::arg("output_path"),
         py::arg("num_threads")=1,
         py::arg("verbose")=false,
         py::arg("training")=true,
         py::arg("tokens_delimiter")=" ",
         py::call_guard<py::gil_scoped_release>())
    .def("detokenize_file", &TokenizerWrapper::detokenize_file,
         py::arg("input_path"),
         py::arg("output_path"),
         py::arg("tokens_delimiter")=" ",
         py::call_guard<py::gil_scoped_release>())

    .def("serialize_tokens", &TokenizerWrapper::serialize_tokens,
         py::arg("tokens"))
    .def("deserialize_tokens", &TokenizerWrapper::deserialize_tokens,
         py::arg("tokens"),
         py::arg("features")=py::none(),
         py::call_guard<py::gil_scoped_release>())

    .def("__copy__", [](const TokenizerWrapper& wrapper) {
      return wrapper;
    })
    .def("__deepcopy__", [](const TokenizerWrapper& wrapper, const py::object& dict) {
      return wrapper;
    })
    ;

  py::class_<SentencePieceTokenizerWrapper, TokenizerWrapper>(m, "SentencePieceTokenizer")
    .def(py::init<const std::string&, const std::optional<std::string>&, int, int, float>(),
         py::arg("model_path"),
         py::arg("vocabulary_path")=py::none(),
         py::arg("vocabulary_threshold")=0,
         py::arg("nbest_size")=0,
         py::arg("alpha")=0.1)
    ;

  py::class_<SubwordLearnerWrapper>(m, "SubwordLearner")
    .def("ingest", &SubwordLearnerWrapper::ingest, py::arg("text"))
    .def("ingest_file", &SubwordLearnerWrapper::ingest_file,
         py::arg("path"),
         py::call_guard<py::gil_scoped_release>())
    .def("ingest_token",
         py::overload_cast<const std::string&>(&SubwordLearnerWrapper::ingest_token),
         py::arg("token"))
    .def("ingest_token",
         py::overload_cast<const onmt::Token&>(&SubwordLearnerWrapper::ingest_token),
         py::arg("token"))
    .def("learn", &SubwordLearnerWrapper::learn,
         py::arg("model_path"),
         py::arg("verbose")=false,
         py::call_guard<py::gil_scoped_release>())
    ;

  py::class_<BPELearnerWrapper, SubwordLearnerWrapper>(m, "BPELearner")
    .def(py::init<const std::optional<TokenizerWrapper>&, int, int, bool>(),
         py::arg("tokenizer")=py::none(),
         py::arg("symbols")=10000,
         py::arg("min_frequency")=2,
         py::arg("total_symbols")=false)
    ;

  py::class_<SentencePieceLearnerWrapper, SubwordLearnerWrapper>(m, "SentencePieceLearner")
    .def(py::init<const std::optional<TokenizerWrapper>&, bool, py::kwargs>(),
         py::arg("tokenizer")=py::none(),
         py::arg("keep_vocab")=false)
    ;
}
