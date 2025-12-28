#include <gtest/gtest.h>

#include <onmt/BPE.h>
#include <onmt/SentencePiece.h>
#include <onmt/Tokenizer.h>

#include <unicode/unistr.h>
#include <unicode/normalizer2.h>

using namespace onmt;

static std::string normalize_nfc(const std::string& s) {
  UErrorCode error = U_ZERO_ERROR;
  const auto* norm = icu::Normalizer2::getNFCInstance(error);
  EXPECT_TRUE(U_SUCCESS(error));

  icu::UnicodeString u = icu::UnicodeString::fromUTF8(s);
  icu::UnicodeString out;
  norm->normalize(u, out, error);
  EXPECT_TRUE(U_SUCCESS(error));

  std::string result;
  out.toUTF8String(result);
  return result;
}

static std::string data_dir;

static std::string get_data(const std::string& path) {
  return data_dir + "/" + path;
}

static void test_tok(const Tokenizer& tokenizer,
                     const std::string& in,
                     const std::string& expected,
                     bool detokenize = false,
                     bool training = true) {
  std::vector<std::string> tokens;
  std::vector<std::vector<std::string>> features;
  tokenizer.tokenize(in, tokens, features, training);
  EXPECT_EQ(write_tokens(tokens, features), expected);
  if (detokenize) {
    EXPECT_EQ(tokenizer.detokenize(tokens, features), in);
  }
}

static void test_tok(const Tokenizer::Options& options,
                     const std::string& in,
                     const std::string& expected,
                     bool detokenize = false,
                     bool training = true) {
  test_tok(Tokenizer(options), in, expected, detokenize, training);
}

static void test_tok(const Tokenizer& tokenizer,
                     const std::string& in,
                     const std::vector<std::string>& expected,
                     bool detokenize = false) {
  std::vector<std::string> tokens;
  tokenizer.tokenize(in, tokens);
  ASSERT_EQ(tokens.size(), expected.size());
  for (size_t i = 0; i < tokens.size(); ++i) {
    EXPECT_EQ(tokens[i], expected[i])  << "Unexpected token mismatch at index " << i;
  }
  if (detokenize) {
    auto text = tokenizer.detokenize(tokens);
    EXPECT_EQ(text, in);
  }
}

static void test_tok(const Tokenizer::Options& options,
                     const std::string& in,
                     const std::vector<std::string>& expected,
                     bool detokenize = false) {
  return test_tok(Tokenizer(options), in, expected, detokenize);
}

static void test_detok(const Tokenizer& tokenizer,
                       const std::string& in,
                       const std::string& expected) {
  std::vector<std::string> tokens;
  std::vector<std::vector<std::string>> features;
  read_tokens(in, tokens, features);
  EXPECT_EQ(
    normalize_nfc(tokenizer.detokenize(tokens, features)),
    normalize_nfc(expected)
  );
}

static void test_detok(const Tokenizer::Options& options,
                       const std::string& in,
                       const std::string& expected) {
  return test_detok(Tokenizer(options), in, expected);
}

static void test_tok_alphabet(const Tokenizer::Options& options,
                              const std::string& in,
                              const std::string& expected,
                              const std::unordered_map<std::string, size_t>& expected_alphabets) {
  std::vector<std::string> words;
  std::vector<std::vector<std::string> > features;
  std::unordered_map<std::string, size_t> alphabets;

  Tokenizer(options).tokenize(in, words, features, alphabets);

  std::string output;
  for (size_t i = 0; i < words.size(); ++i)
  {
    if (i > 0)
      output += " ";
    output += words[i];
  }

  EXPECT_EQ(output, expected);

  for (const auto& it: expected_alphabets) {
    ASSERT_TRUE(alphabets.find(it.first) != alphabets.end());
    EXPECT_EQ(alphabets[it.first], it.second);
  }
}

static void test_tok_and_detok(const Tokenizer& tokenizer,
                               const std::string& in,
                               const std::string& expected) {
  return test_tok(tokenizer, in, expected, true);
}

static void test_tok_and_detok(const Tokenizer::Options& options,
                               const std::string& in,
                               const std::string& expected) {
  return test_tok_and_detok(Tokenizer(options), in, expected);
}


TEST(TokenizerTest, DetokenizeEmptyToken) {
  Tokenizer tokenizer({});
  EXPECT_EQ(tokenizer.detokenize({"a", "", "b"}), "a b");
}

TEST(TokenizerTest, DetokenizeWithRanges) {
  Tokenizer tokenizer({});
  Ranges ranges;
  tokenizer.detokenize({"Hel￭", "lo", "｟mrk_case_modifier_C｠", "w", "￭", "orld", "￭!"}, ranges);
  // Result: Hello World!
  ASSERT_EQ(ranges.size(), 5);
  EXPECT_EQ(ranges[0], (std::pair<size_t, size_t>(0, 2)));
  EXPECT_EQ(ranges[1], (std::pair<size_t, size_t>(3, 4)));
  EXPECT_EQ(ranges[3], (std::pair<size_t, size_t>(6, 6)));
  EXPECT_EQ(ranges[5], (std::pair<size_t, size_t>(7, 10)));
  EXPECT_EQ(ranges[6], (std::pair<size_t, size_t>(11, 11)));
}

TEST(TokenizerTest, DetokenizeWithMergedRanges) {
  Tokenizer tokenizer({});
  Ranges ranges;
  tokenizer.detokenize({"Hel￭", "lo", "w￭", "orld", "￭!"}, ranges, true);
  // Result: Hello World!
  ASSERT_EQ(ranges.size(), 5);
  EXPECT_EQ(ranges[0], (std::pair<size_t, size_t>(0, 4)));
  EXPECT_EQ(ranges[1], (std::pair<size_t, size_t>(0, 4)));
  EXPECT_EQ(ranges[2], (std::pair<size_t, size_t>(6, 10)));
  EXPECT_EQ(ranges[3], (std::pair<size_t, size_t>(6, 10)));
  EXPECT_EQ(ranges[4], (std::pair<size_t, size_t>(11, 11)));
}

TEST(TokenizerTest, DetokenizeWithMergedRangesPlaceholders) {
  Tokenizer tokenizer({});
  Ranges ranges;
  tokenizer.detokenize({"｟a｠￭", "b", "￭｟c｠"}, ranges, true);
  // Result: ｟a｠b｟c｠
  ASSERT_EQ(ranges.size(), 3);
  EXPECT_EQ(ranges[0], (std::pair<size_t, size_t>(0, 6)));
  EXPECT_EQ(ranges[1], (std::pair<size_t, size_t>(7, 7)));
  EXPECT_EQ(ranges[2], (std::pair<size_t, size_t>(8, 14)));
}

TEST(TokenizerTest, Empty) {
  test_tok({}, "", "");
}

TEST(TokenizerTest, NonbreakableSpace) {
  test_tok({}, "a b", "a b");
}

TEST(TokenizerTest, Conservative) {
  test_tok({},
           "Your Hardware-Enablement Stack (HWE) is supported until April 2019.",
           "Your Hardware-Enablement Stack ( HWE ) is supported until April 2019 .");
}

TEST(TokenizerTest, None) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  test_tok(options, "Hello World!", "Hello World!");
}

TEST(TokenizerTest, NoneWithPlaceholders) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  test_tok(options, "Hello:｟World｠!", "Hello:￭ ｟World｠￭ !");
}

TEST(TokenizerTest, NonePlaceholderSpacesEscape) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  test_tok(options, "｟a b c｠", "｟a％0020b％0020c｠");
}

TEST(TokenizerTest, NonePlaceholderSpacesNoEscape) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.no_substitution = true;
  test_tok(options, "｟a b c｠", "｟a b c｠");
}

TEST(TokenizerTest, NonePreservePlaceholders) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  options.preserve_placeholders = true;
  test_tok(options, "Hello:｟World｠!", "Hello:￭ ｟World｠ ￭ !");
}

TEST(TokenizerTest, NonePreserveTokens) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  options.preserve_segmented_tokens = true;
  test_tok(options, "Hello｟World｠!", "Hello ￭ ｟World｠ ￭ !");
}

TEST(TokenizerTest, Space) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  test_tok(options,
           "49th meeting Social and human rights questions: human rights [14 (g)]",
           "49th meeting Social and human rights questions: human rights [14 (g)]");
}

TEST(TokenizerTest, SpaceEmpty) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  test_tok(options, "", "");
}

TEST(TokenizerTest, SpaceSingle) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  test_tok(options, "Hello", "Hello");
}

TEST(TokenizerTest, SpaceLeading) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  test_tok(options, " Hello", "Hello");
}

TEST(TokenizerTest, SpaceTrailing) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  test_tok(options, "Hello ", "Hello");
}

TEST(TokenizerTest, SpaceDuplicated) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  test_tok(options, "  Hello   World ", "Hello World");
}

TEST(TokenizerTest, SpacePlaceholderSpacesEscape) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner_annotate = true;
  test_tok(options, "a｟b c｠ d", "a￭ ｟b％0020c｠ d");
}

TEST(TokenizerTest, SpaceWithFeatures) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.case_feature = true;
  Tokenizer tokenizer(options);
  std::vector<std::string> tokens;
  std::vector<std::vector<std::string>> features;
  tokenizer.tokenize("Hello￨12￨AB world￨34￨CD", tokens, features);
  EXPECT_EQ(tokens, (std::vector<std::string>{"hello", "world"}));
  ASSERT_EQ(features.size(), 3);
  EXPECT_EQ(features[0], (std::vector<std::string>{"12", "34"}));
  EXPECT_EQ(features[1], (std::vector<std::string>{"AB", "CD"}));
  EXPECT_EQ(features[2], (std::vector<std::string>{"C", "L"}));
}

TEST(TokenizerTest, Char) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Char;
  test_tok(options, "  Hello   World 123.", "H e l l o W o r l d 1 2 3 .");
}

TEST(TokenizerTest, CharWithSpacer) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Char;
  options.spacer_annotate = true;
  test_tok(options, "  Hello   World 123.", "H e l l o ▁W o r l d ▁1 2 3 .");
}

TEST(TokenizerTest, CharWithSpacerNew) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Char;
  options.spacer_annotate = true;
  options.spacer_new = true;
  test_tok(options, "  Hello   World 123.", "H e l l o ▁ W o r l d ▁ 1 2 3 .");
}

TEST(TokenizerTest, JoinerAnnotate) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  test_tok_and_detok(options,
                     "Isn't it so-greatly working?",
                     "Isn ￭'￭ t it so ￭-￭ greatly working ￭?");
  test_tok_and_detok(options, "MP3", "MP ￭3");
  test_tok_and_detok(options, "A380", "A ￭380");
  test_tok_and_detok(options, "$1", "$￭ 1");
}

TEST(TokenizerTest, PriorJoinerSupportSpace) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner_annotate = true;
  options.support_prior_joiners = true;
  options.preserve_placeholders = true;
  test_tok(options,
           "It is a test-aggressive ￭'￭ with pre￭ tokenizat ￭ions World￭ 123 and double ￭｟mrk_place｠｟mrk_holder｠￭ .",
           "It is a test-aggressive ￭'￭ with pre￭ tokenizat ￭ions World￭ 123 and double ￭ ｟mrk_place｠ ￭ ｟mrk_holder｠ ￭ .");
}

TEST(TokenizerTest, PriorJoinerSupport) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  options.support_prior_joiners = true;
  test_tok(options,
           "It is a test-aggressive ￭'￭ with pre￭ tokenizat ￭ions World￭ 123.",
           "It is a test ￭-￭ aggressive ￭'￭ with pre￭ tokenizat ￭ions World￭ 123 ￭.");
}

TEST(TokenizerTest, PriorJoinerAndPlaceholder) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  options.support_prior_joiners = true;
  test_tok(options, "｟a￭b｠", "｟a￭b｠");
}

TEST(TokenizerTest, SpacerAnnotate) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.spacer_annotate = true;
  test_tok_and_detok(options,
                     "Isn't it so-greatly working?",
                     "Isn ' t ▁it ▁so - greatly ▁working ?");
  test_tok_and_detok(options, "MP3", "MP 3");
}

TEST(TokenizerTest, SpacerNew) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.spacer_annotate = true;
  options.spacer_new = true;
  test_tok_and_detok(options,
                     "Isn't it so-greatly working?",
                     "Isn ' t ▁ it ▁ so - greatly ▁ working ?");
}

TEST(TokenizerTest, ProtectedSequence) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  test_tok_and_detok(options, "｟1,023｠km", "｟1,023｠￭ km");
  test_tok_and_detok(options, "A｟380｠", "A ￭｟380｠");
  test_tok_and_detok(options, "｟1,023｠｟380｠", "｟1,023｠￭ ｟380｠");
  test_tok_and_detok(options, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(options, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(options, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(options, "｟US$｠23", "｟US$｠￭ 23");
  test_tok_and_detok(options, "1｟ABCD｠0", "1 ￭｟ABCD｠￭ 0");
}

TEST(TokenizerTest, PreserveProtectedSequence) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.preserve_placeholders = true;
  test_tok_and_detok(options, "｟1,023｠km", "｟1,023｠ ￭ km");
  test_tok_and_detok(options, "A｟380｠", "A ￭ ｟380｠");
  test_tok_and_detok(options, "｟1,023｠｟380｠", "｟1,023｠ ￭ ｟380｠");
  test_tok_and_detok(options, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(options, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(options, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(options, "｟US$｠23", "｟US$｠ ￭ 23");
  test_tok_and_detok(options, "1｟ABCD｠0", "1 ￭ ｟ABCD｠ ￭ 0");
}

TEST(TokenizerTest, PreserveProtectedSequenceSpacerAnnotate) {
  Tokenizer::Options options;
  options.spacer_annotate = true;
  options.preserve_placeholders = true;
  test_tok_and_detok(options, "｟1,023｠km", "｟1,023｠ km");
  test_tok_and_detok(options, "A ｟380｠", "A ▁ ｟380｠");
  test_tok_and_detok(options, "｟1,023｠｟380｠", "｟1,023｠ ｟380｠");
  test_tok_and_detok(options, "｟1,023｠ ｟380｠", "｟1,023｠ ▁ ｟380｠");
  test_tok_and_detok(options, "｟1023｠.", "｟1023｠ .");
  test_tok_and_detok(options, "｟1023｠ .", "｟1023｠ ▁.");
}

TEST(TokenizerTest, ProtectedSequenceAggressive) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  test_tok_and_detok(options, "｟1,023｠km", "｟1,023｠￭ km");
  test_tok_and_detok(options, "A｟380｠", "A ￭｟380｠");
  test_tok_and_detok(options, "｟1,023｠｟380｠", "｟1,023｠￭ ｟380｠");
  test_tok_and_detok(options, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(options, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(options, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(options, "｟US$｠23", "｟US$｠￭ 23");
  test_tok_and_detok(options, "1｟ABCD｠0", "1 ￭｟ABCD｠￭ 0");
}

TEST(TokenizerTest, ProtectedSequenceJoinerNew) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.joiner_new = true;
  test_tok_and_detok(options, "｟1,023｠km", "｟1,023｠ ￭ km");
  test_tok_and_detok(options, "A｟380｠", "A ￭ ｟380｠");
  test_tok_and_detok(options, "｟1,023｠｟380｠", "｟1,023｠ ￭ ｟380｠");
  test_tok_and_detok(options, "｟1023｠.", "｟1023｠ ￭ .");
}

TEST(TokenizerTest, Substitution) {
  test_tok({},
           "test￭ protect￨, ：, ▁, and ％ or ＃...",
           "test ■ protect │ , : , _ , and % or # . . .");
  test_tok({}, "｟tag：value with spaces｠", "｟tag：value％0020with％0020spaces｠");
}

TEST(TokenizerTest, NoSubstitution) {
  Tokenizer::Options options;
  options.no_substitution = true;
  test_tok(options,
           "test￭ protect￨, ：, ▁, and ％ or ＃...",
           "test ￭ protect ￨ , ： , ▁ , and ％ or ＃ . . .");
  test_tok(options, "｟tag：value with spaces｠", "｟tag：value with spaces｠");
}

TEST(TokenizerTest, WithSeparators) {
  Tokenizer::Options options;
  options.with_separators = true;
  test_tok(options, "Hello World!", {"Hello", " ", "World", "!"}, true);
  test_detok(options, "Hello   World !", "Hello World!");
  test_detok(options, "Hello     World !", "Hello  World!");
}

TEST(TokenizerTest, JoinerSubstitution) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner_annotate = true;
  test_tok(options,
           "It is a test-aggressive ■'■ with pre￭ tokenizat ■ions ｟entity＃1：World￭｠ 123.",
           "It is a test-aggressive ■'■ with pre■ tokenizat ■ions ｟entity＃1：World￭｠ 123.");
}

TEST(TokenizerTest, InvalidEscapeSequence) {
  test_detok({},
             "要求从１４％降到２％，动员干部参加生产，向合作社看齐。",
             "要求从１４％降到２％，动员干部参加生产，向合作社看齐。");
  test_detok({},
             "王鴻薇說，復星僅持有0.7％BNT股權，所以德國BNT是德國公司無誤",
             "王鴻薇說，復星僅持有0.7％BNT股權，所以德國BNT是德國公司無誤");
}

TEST(TokenizerTest, ZeroWidthJoiner) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  test_tok_and_detok(options, "👨‍👩‍👦", "👨 ￭‍ ￭👩 ￭‍ ￭👦");
}

TEST(TokenizerTest, CombiningMark) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  test_tok_and_detok(options,
                     "वर्तमान लिपि (स्क्रिप्ट) खो जाएगी।",
                     "वर्तमान लिपि (￭ स्क्रिप्ट ￭) खो जाएगी ￭।");
}

TEST(TokenizerTest, CombiningMarkOnSpace) {
  {
    Tokenizer::Options options;
    options.joiner_annotate = true;
    test_tok_and_detok(options, "b ̇c", "b ￭％0020̇￭ c");
  }

  {
    Tokenizer::Options options;
    options.joiner_annotate = true;
    options.allow_isolated_marks = true;
    test_tok_and_detok(options, "b ̇c", "b ̇￭ c");
  }

  {
    Tokenizer::Options options;
    options.spacer_annotate = true;
    test_tok_and_detok(options, "b ̇c", "b ％0020̇ c");
  }
}

TEST(TokenizerTest, CombiningMarkOnSpaceNoSubstitution) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.no_substitution = true;
  test_tok(options, "angles ၧ1 and ၧ2", {"angles", "￭ ၧ￭", "1", "and", "￭ ၧ￭", "2"}, true);
}

TEST(TokenizerTest, CombiningMarkAfterPlaceholder) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.preserve_placeholders = true;
  test_tok_and_detok(options, "｟a｠ׂb", "｟a｠ ￭ׂ￭ b");
}

TEST(TokenizerTest, Alphabets) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.segment_alphabet_change = true;
  test_tok_alphabet(options, "rawБ", "raw Б", {{"Latin", 3}, {"Cyrillic", 1}});
  test_tok_alphabet(options, "有 入", "有 入", {{"Han", 2}});
}

TEST(TokenizerTest, ArabicAlphabet) {
  test_tok_alphabet({}, "مرحبا", "مرحبا", {{"Arabic", 5}});
}

TEST(TokenizerTest, HalfWidthKanaAlphabet) {
  test_tok_alphabet({}, "ﾃ", "ﾃ", {{"Katakana", 1}});
}

TEST(TokenizerTest, CaseFeature) {
  Tokenizer::Options options;
  options.case_feature = true;
  options.joiner_annotate = true;
  // Note: in C literal strings, \ is escaped by another \.
  test_tok(options,
           "test \\\\\\\\a Capitalized lowercased UPPERCASÉ miXêd - cyrillic-Б",
           "test￨L \\￨N ￭\\￨N ￭\\￨N ￭\\￭￨N a￨L capitalized￨C lowercased￨L uppercasé￨U mixêd￨M -￨N cyrillic-б￨M");
}

TEST(TokenizerTest, CaseFeatureWithJoinerNew) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.case_feature = true;
  options.joiner_annotate = true;
  options.joiner_new = true;
  test_tok(options, "a-b.", "a￨L ￭￨N -￨N ￭￨N b￨L ￭￨N .￨N");
}

TEST(TokenizerTest, CaseFeatureWithSpacerNew) {
  Tokenizer::Options options;
  options.case_feature = true;
  options.spacer_annotate = true;
  options.spacer_new = true;
  test_tok(options, "a b", "a￨L ▁￨N b￨L");
}

TEST(TokenizerTest, CaseMarkupWithJoiners) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.joiner_annotate = true;
  test_tok_and_detok(options,
                     "Hello world!", "｟mrk_case_modifier_C｠ hello world ￭!");
  test_tok_and_detok(options,
                     "Hello WORLD!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ world ｟mrk_end_case_region_U｠ ￭!");
  test_tok_and_detok(options,
                     "HELLO WORLD!", "｟mrk_begin_case_region_U｠ hello ｟mrk_end_case_region_U｠ ｟mrk_begin_case_region_U｠ world ｟mrk_end_case_region_U｠ ￭!");
  test_tok_and_detok(options,
                     "Hello WOrld!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ wo￭ ｟mrk_end_case_region_U｠ rld ￭!");
  test_tok_and_detok(options,
                     "hello woRld!", "hello wo￭ ｟mrk_case_modifier_C｠ rld ￭!");
  test_tok_and_detok(options,
                     "hello woRlD!", "hello wo￭ ｟mrk_case_modifier_C｠ rl￭ ｟mrk_case_modifier_C｠ d ￭!");
}

TEST(TokenizerTest, CaseMarkupWithSoftUppercaseRegions) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.case_markup = true;
  options.soft_case_regions = true;
  options.joiner_annotate = true;
  test_tok_and_detok(options,
                     "AA.BB", "｟mrk_begin_case_region_U｠ aa ￭.￭ bb ｟mrk_end_case_region_U｠");
  test_tok_and_detok(options,
                     "A BC", "｟mrk_begin_case_region_U｠ a bc ｟mrk_end_case_region_U｠");
  test_tok_and_detok(options,
                     "AA.", "｟mrk_begin_case_region_U｠ aa ｟mrk_end_case_region_U｠ ￭.");
  test_tok_and_detok(options,
                     "A-B/C", "｟mrk_begin_case_region_U｠ a ￭-￭ b ￭/￭ c ｟mrk_end_case_region_U｠");
  test_tok_and_detok(options,
                     "A-B/c", "｟mrk_begin_case_region_U｠ a ￭-￭ b ｟mrk_end_case_region_U｠ ￭/￭ c");
  test_tok_and_detok(options,
                     "A", "｟mrk_case_modifier_C｠ a");
  test_tok_and_detok(options,
                     "A-", "｟mrk_case_modifier_C｠ a ￭-");
  test_tok_and_detok(options,
                     "ID: A23X52,",
                     "｟mrk_begin_case_region_U｠ id ￭: a ￭23￭ x ￭52 ｟mrk_end_case_region_U｠ ￭,");
  test_tok_and_detok(options,
                     "Show PP-LX-DP",
                     "｟mrk_case_modifier_C｠ show ｟mrk_begin_case_region_U｠ pp ￭-￭ lx ￭-￭ dp ｟mrk_end_case_region_U｠");
  test_tok_and_detok(options,
                     "AA ｟BB｠ CC", "｟mrk_begin_case_region_U｠ aa ｟mrk_end_case_region_U｠ ｟BB｠ ｟mrk_begin_case_region_U｠ cc ｟mrk_end_case_region_U｠");
}

TEST(TokenizerTest, CaseMarkupWithJoinerNew) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.joiner_annotate = true;
  options.joiner_new = true;
  test_detok(options, "hello ｟mrk_case_modifier_C｠ ￭ world !", "helloWorld !");
  test_detok(options, "hello ｟mrk_case_modifier_C｠ ￭", "hello");
}

TEST(TokenizerTest, CaseMarkupWithSpacers) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.spacer_annotate = true;
  test_tok_and_detok(options,
                     "Hello world!", "｟mrk_case_modifier_C｠ hello ▁world !");
  test_tok_and_detok(options,
                     "Hello WORLD!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ ▁world ｟mrk_end_case_region_U｠ !");
  test_tok_and_detok(options,
                     "Hello WOrld!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ ▁wo ｟mrk_end_case_region_U｠ rld !");
  test_tok_and_detok(options,
                     "hello woRld!", "hello ▁wo ｟mrk_case_modifier_C｠ rld !");
}

TEST(TokenizerTest, CaseMarkupWithSpacerNew) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.spacer_annotate = true;
  options.spacer_new = true;
  test_detok(options, "hello ｟mrk_case_modifier_C｠ ▁ world !", "hello World!");
  test_detok(options, "hello ｟mrk_case_modifier_C｠ ▁", "hello ");
}

TEST(TokenizerTest, CaseMarkupWithBPE) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.joiner_annotate = true;
  Tokenizer tokenizer(options,
                      std::make_shared<BPE>(get_data("bpe-models/codes_suffix_case_insensitive.fr")));
  test_tok_and_detok(tokenizer,
                     "Bonjour monde", "｟mrk_case_modifier_C｠ bon￭ j￭ our mon￭ de");
  test_tok_and_detok(tokenizer,
                     "BONJOUR MONDE", "｟mrk_begin_case_region_U｠ bon￭ j￭ our ｟mrk_end_case_region_U｠ ｟mrk_begin_case_region_U｠ mon￭ de ｟mrk_end_case_region_U｠");
  test_tok_and_detok(tokenizer,
                     "BONJOUR monde", "｟mrk_begin_case_region_U｠ bon￭ j￭ our ｟mrk_end_case_region_U｠ mon￭ de");
}

TEST(TokenizerTest, CaseMarkupWithMixedScripts) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.joiner_annotate = true;
  test_tok_and_detok(options,
                     "「我々は、うまくやっていることを証明した」とヒョンCEOは話す。",
                     "「￭ 我々は ￭、￭ うまくやっていることを証明した ￭」￭ とヒョン￭ ｟mrk_begin_case_region_U｠ ceo￭ ｟mrk_end_case_region_U｠ は話す ￭。");
}

TEST(TokenizerTest, CaseMarkupDetokenizationWithPlaceholders) {
  Tokenizer::Options options;
  options.case_markup = true;
  test_detok(options, "｟mrk_case_modifier_C｠ ｟abc｠", "｟abc｠");
  test_detok(options, "｟mrk_begin_case_region_U｠ ｟abc｠ ｟mrk_end_case_region_U｠", "｟abc｠");
}

TEST(TokenizerTest, CaseMarkupDetokenizationWithMissingModifiedToken) {
  Tokenizer::Options options;
  options.case_markup = true;
  test_detok(options, "hello ｟mrk_case_modifier_C｠", "hello");
  test_detok(options, "｟mrk_case_modifier_C｠ ｟mrk_case_modifier_C｠ hello", "Hello");
  test_detok(options, "｟mrk_case_modifier_C｠ ｟mrk_begin_case_region_U｠ hello ｟mrk_end_case_region_U｠", "HELLO");
}

TEST(TokenizerTest, CaseMarkupDetokenizationWithMissingRegionMarker) {
  Tokenizer::Options options;
  options.case_markup = true;
  test_detok(options, "｟mrk_begin_case_region_U｠ hello", "HELLO");
  test_detok(options,
             "｟mrk_begin_case_region_U｠ hello ｟mrk_case_modifier_C｠ world", "HELLO World");
  test_detok(options,
             "｟mrk_end_case_region_U｠ hello ｟mrk_case_modifier_C｠ world", "hello World");
}

TEST(TokenizerTest, CaseMarkupDetokenizationWithNestedMarkers) {
  Tokenizer::Options options;
  options.case_markup = true;
  test_detok(options,
             "｟mrk_begin_case_region_U｠ ｟mrk_case_modifier_C｠ hello world ｟mrk_end_case_region_U｠", "Hello WORLD");
  test_detok(options,
             "｟mrk_begin_case_region_U｠ hello ｟mrk_case_modifier_C｠ ｟mrk_end_case_region_U｠ world", "HELLO world");
}

TEST(TokenizerTest, CaseMarkupWithLocaleEl) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.soft_case_regions = true;
  options.lang = "el";
  test_tok(options,
           "ΣΙΓΜΑ ΤΕΛΙΚΟΣ",
           "｟mrk_begin_case_region_U｠ σιγμα τελικος ｟mrk_end_case_region_U｠");
  test_detok(options,
             "｟mrk_begin_case_region_U｠ την άνοιξη , απρίλιο ή μάιο , θα καταναλώσω μεγαλύτερες ποσότητες πρωτεΐνης ｟mrk_end_case_region_U｠",
             "ΤΗΝ ΑΝΟΙΞΗ , ΑΠΡΙΛΙΟ Ή ΜΑΪΟ , ΘΑ ΚΑΤΑΝΑΛΩΣΩ ΜΕΓΑΛΥΤΕΡΕΣ ΠΟΣΟΤΗΤΕΣ ΠΡΩΤΕΪΝΗΣ");
}

TEST(TokenizerTest, CaseMarkupWithLocaleNl) {
  Tokenizer::Options options;
  options.case_markup = true;
  options.lang = "nl";
  test_detok(options, "｟mrk_case_modifier_C｠ ijssel", "IJssel");
}

TEST(TokenizerTest, SegmentCase) {
  Tokenizer::Options options;
  options.case_feature = true;
  options.joiner_annotate = true;
  options.segment_case = true;
  test_tok_and_detok(options, "WiFi", "wi￭￨C fi￨C");
}

TEST(TokenizerTest, SegmentNumbers) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  options.segment_numbers = true;
  test_tok_and_detok(options,
                     "1984 mille neuf cent quatrevingt-quatre",
                     "1￭ 9￭ 8￭ 4 mille neuf cent quatrevingt ￭-￭ quatre");
}

TEST(TokenizerTest, SegmentAlphabet) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.segment_alphabet = {"Han"};
  test_tok_and_detok(options, "rawБ", "rawБ");
  test_tok(options,
           "有入聲嘅唐話往往有陽入對轉，即係入聲韻尾同鼻音韻尾可以轉化。比如粵語嘅「抌」（dam）「揼」（dap），意思接近，意味微妙，區別在於-m同-p嘅轉換。",
           "有￭ 入￭ 聲￭ 嘅￭ 唐￭ 話￭ 往￭ 往￭ 有￭ 陽￭ 入￭ 對￭ 轉 ￭，￭ 即￭ 係￭ 入￭ 聲￭ 韻￭ 尾￭ 同￭ 鼻￭ 音￭ 韻￭ 尾￭ 可￭ 以￭ 轉￭ 化 ￭。￭ 比￭ 如￭ 粵￭ 語￭ 嘅 ￭「￭ 抌 ￭」 ￭（￭ dam ￭） ￭「￭ 揼 ￭」 ￭（￭ dap ￭） ￭，￭ 意￭ 思￭ 接￭ 近 ￭，￭ 意￭ 味￭ 微￭ 妙 ￭，￭ 區￭ 別￭ 在￭ 於-m同-p嘅￭ 轉￭ 換 ￭。");
}

// Checking backward compatibility with the "Kanbun" and "Kangxi" alphabets that are not
// included in ICU list of Unicode script aliases.
TEST(TokenizerTest, SegmentAlphabetKangxi) {
  Tokenizer::Options options;
  options.segment_alphabet = {"Kangxi"};
  test_tok(options, "12⼀⼁", "12 ⼀ ⼁");
}
TEST(TokenizerTest, SegmentAlphabetKanbun) {
  Tokenizer::Options options;
  options.segment_alphabet = {"Kanbun"};
  test_tok(options, "12㆙㆚", "12 ㆙ ㆚");
}

TEST(TokenizerTest, SegmentAlphabetChange) {
  Tokenizer::Options options;
  options.segment_alphabet_change = true;
  test_tok(options, "rawБ", "raw Б");
}

TEST(TokenizerTest, SegmentAlphabetChangeCommonScript) {
  Tokenizer::Options options;
  options.segment_alphabet_change = true;
  // Character ー can appear in both Hiragana and Katakana and should not be segmented when
  // appearing in these contexts. See https://github.com/OpenNMT/Tokenizer/issues/210.
  test_tok(options, "「キャント・バイ・ミー・ラヴ」", "「 キャント ・ バイ ・ ミー ・ ラヴ 」");
}

TEST(TokenizerTest, SegmentAlphabetChangeIsolatedMarks) {
  Tokenizer::Options options;
  options.segment_alphabet_change = true;
  options.allow_isolated_marks = true;
  options.joiner_annotate = true;
  test_tok(options, "abc়", "abc ￭়");
  test_tok(options, "8ে", "8 ￭ে");
  test_tok(options, "ё", "ё");  // combining mark with inherited script.

  options.preserve_segmented_tokens = true;
  test_tok(options, "abc়", "abc ￭ ়");
}

TEST(TokenizerTest, PreserveSegmentedNumbers) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  options.segment_numbers = true;
  options.preserve_segmented_tokens = true;
  test_tok_and_detok(options, "1234", "1 ￭ 2 ￭ 3 ￭ 4");
}

TEST(TokenizerTest, PreserveSegmentAlphabetChange) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.segment_alphabet_change = true;
  options.preserve_segmented_tokens = true;
  test_tok_and_detok(options, "rawБ", "raw ￭ Б");
}

TEST(TokenizerTest, PreserveSegmentAlphabet) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  options.segment_alphabet = {"Han"};
  options.segment_alphabet_change = true;
  options.preserve_segmented_tokens = true;
  test_tok_and_detok(options, "測試abc", "測 ￭ 試 ￭ abc");
}

TEST(TokenizerTest, PreserveSegmentCase) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.segment_case = true;
  options.preserve_segmented_tokens = true;
  test_tok_and_detok(options, "WiFi", "Wi ￭ Fi");
}

TEST(TokenizerTest, PreserveSegmentCaseBPE) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.segment_case = true;
  options.preserve_segmented_tokens = true;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/fr500")));
  test_tok_and_detok(tokenizer, "BonjourMonde", "B￭ on￭ jou￭ r ￭ M￭ on￭ de");
  test_tok_and_detok(tokenizer, "aB", "a ￭ B");
}

TEST(TokenizerTest, BPE) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/testcode.v0.1")));
  test_tok_and_detok(tokenizer,
                     "abcdimprovement联合国",
                     "a￭ b￭ c￭ d￭ impr￭ ovemen￭ t￭ 联合￭ 国");
}

TEST(TokenizerTest, BPEModePrefix) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/codes_prefix.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S e u lement seulement il v ais n on se u lement seulement n on à V er d un");
}

TEST(TokenizerTest, BPEModeNofix) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/codes_nofix.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S e u lement seulement il v ais n on seulement seulement n on à V er d un");
}

TEST(TokenizerTest, BPEModeBoth) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/codes_bothfix.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S eu lement seulement il va is n on s eu lement seu l emen t n on à V er du n");
}

TEST(TokenizerTest, BPECaseInsensitive) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/codes_suffix_case_insensitive.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "Seulement seulement il va is n on seulement seu l em ent n on à Ver d un");
}

TEST(TokenizerTest, BPEDropout) {
  Tokenizer tokenizer({}, std::make_shared<BPE>(get_data("bpe-models/codes_suffix_case_insensitive.fr"), 1.0));
  test_tok(tokenizer, "seulement", "s e u l e m e n t");
  test_tok(tokenizer, "seulement", "seulement", /*detokenize=*/false, /*training=*/false);
}

TEST(TokenizerTest, BPEVocabularyWithTrailingJoiner) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner_annotate = true;
  options.support_prior_joiners = true;
  auto bpe = std::make_shared<BPE>(get_data("bpe-models/bpe_code.v0.2"));
  bpe->set_vocabulary({"wel￭"});
  Tokenizer tokenizer(options, bpe);
  test_tok(tokenizer, "wel￭ le", "wel￭ l￭ e");
  test_tok(tokenizer, "wel le", "w￭ e￭ l l￭ e");
}

TEST(TokenizerTest, BPEVocabularyWithLeadingJoiner) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  auto bpe = std::make_shared<BPE>(get_data("bpe-models/bpe_code.v0.2"));
  bpe->set_vocabulary({"￭10"});
  Tokenizer tokenizer(options, bpe);
  test_tok(tokenizer, "A10", "A ￭10");
  test_tok(tokenizer, "A100", "A ￭1￭ 0￭ 0");
}

TEST(TokenizerTest, BPEVocabularyWithPreservedTokens) {
  Tokenizer::Options options;
  options.joiner_annotate = true;
  options.joiner = "￭";

  BPE bpe(get_data("bpe-models/bpe_code.v0.2"));
  bpe.set_vocabulary({"wel"}, &options);

  Token token("welle");
  token.preserve = true;
  token.join_right = true;
  auto subwords = bpe.encode_and_annotate(token);

  ASSERT_EQ(subwords.size(), 5);

  EXPECT_EQ(subwords[0].surface, "w");
  EXPECT_TRUE(subwords[0].join_right);
  EXPECT_FALSE(subwords[0].preserve);

  EXPECT_EQ(subwords[1].surface, "e");
  EXPECT_TRUE(subwords[1].join_right);
  EXPECT_FALSE(subwords[1].preserve);

  EXPECT_EQ(subwords[2].surface, "l");
  EXPECT_TRUE(subwords[2].join_right);
  EXPECT_FALSE(subwords[2].preserve);
}

TEST(TokenizerTest, BPEVocabularyWithSpacer) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.spacer_annotate = true;

  auto bpe = std::make_shared<BPE>(get_data("bpe-models/bpe_code.v0.2"));
  bpe->set_vocabulary({"▁wel"}, &options);
  Tokenizer tokenizer(options, bpe);

  test_tok(tokenizer, "die welle", "d i e ▁wel l e");
}

TEST(TokenizerTest, BPEWithoutVocabulary) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner = "@@";
  options.joiner_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<BPE>(get_data("bpe-models/bpe_code.v0.2")));
  test_tok(tokenizer, "Oliver Grün , welle", "Oliver Grün , welle");
}

TEST(TokenizerTest, BPEWithVocabulary) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner = "@@";
  options.joiner_annotate = true;
  auto bpe = std::make_shared<BPE>(get_data("bpe-models/bpe_code.v0.2"));
  bpe->load_vocabulary(get_data("bpe-models/vocab.en"), 50, &options);
  Tokenizer tokenizer(options, bpe);
  test_tok(tokenizer, "Oliver Grün , welle", "Oliver Gr@@ ü@@ n , wel@@ le");
}

TEST(TokenizerTest, BPEWithVocabularyTabSeparated) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Space;
  options.joiner = "@@";
  options.joiner_annotate = true;
  auto bpe = std::make_shared<BPE>(get_data("bpe-models/bpe_code.v0.2"));
  bpe->load_vocabulary(get_data("bpe-models/vocab.en.tab"), 50, &options);
  Tokenizer tokenizer(options, bpe);
  test_tok(tokenizer, "Oliver Grün , welle", "Oliver Gr@@ ü@@ n , wel@@ le");
}

TEST(TokenizerTest, SentencePiece) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp.model")));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The ▁two ▁shows , ▁called ▁De si re ▁and ▁S e c re t s , ▁will ▁be ▁one - hour ▁prime - time ▁shows .");
}

TEST(TokenizerTest, SentencePieceWithJoinersAndPlaceholders) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp.model")));
  test_tok_and_detok(tokenizer,
                     "The two shows, called ｟Desire｠ and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called ｟Desire｠ and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
  test_tok_and_detok(tokenizer,
                     "The two shows, called｟Desire｠and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called￭ ｟Desire｠￭ and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
}

TEST(TokenizerTest, SentencePieceWithJoinersAndPreservePlaceholders) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  options.preserve_placeholders = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp.model")));
  test_tok_and_detok(tokenizer,
                     "The two shows, called｟Desire｠and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called￭ ｟Desire｠ ￭ and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
}

TEST(TokenizerTest, SentencePieceSubwordRegularization) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp_regularization.model"), 1, 0.1));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "▁The ▁ two ▁show s , ▁call ed ▁De si re ▁ and ▁Sec re t s , ▁w ill ▁be ▁one - h our ▁ pri me - t im e ▁show s .");
}

TEST(TokenizerTest, SentencePieceAlt) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/wmtende.model")));
  test_tok_and_detok(tokenizer,
                     "Bamford is appealing the sentence and has been granted bail of 50,000 baht.",
                     "▁Ba m ford ▁is ▁appealing ▁the ▁sentence ▁and ▁has ▁been ▁granted ▁bail ▁of ▁ 50,000 ▁ba ht .");
}

TEST(TokenizerTest, SentencePieceLeadingSpacer) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/wmtende.model")));
  test_tok_and_detok(tokenizer, "Experts", "▁ Expert s");
  test_tok_and_detok(tokenizer, "Expert", "▁ Expert");
}

TEST(TokenizerTest, SentencePieceWithJoiners) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp.model")));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called De ￭si ￭re and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
}

TEST(TokenizerTest, SentencePieceAggressive) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/wmtende.model")));
  test_tok(tokenizer,
           "Bamford is appealing the sentence and has been granted bail of 50,000 baht.",
           "Ba m ford is appealing the sentence and has been granted bail of 50 , 000 ba ht .");
}

TEST(TokenizerTest, SentencePieceAggressiveAndSpacers) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.spacer_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp.model")));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The ▁t wo ▁s how s , ▁called ▁D es ir e ▁and ▁Se c re t s , ▁will ▁be ▁one - hour ▁p rime - time ▁s how s .");
}

TEST(TokenizerTest, SentencePieceAggressiveAndJoiners) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/sp.model")));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The t ￭wo s ￭how ￭s ￭, called D ￭es ￭ir ￭e and Se ￭c ￭re ￭t ￭s ￭, will be one ￭-￭ hour p ￭rime ￭-￭ time s ￭how ￭s ￭.");
}

TEST(TokenizerTest, SentencePieceIsolatedSpacer) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.preserve_placeholders = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/wmtende.model")));
  test_tok(tokenizer, "a crumpled sofa", "▁a ▁ cru mpl ed ▁sofa");
  test_tok(tokenizer, "a ｟ph｠crumpled sofa", "▁a ▁ ｟ph｠ cru mpl ed ▁sofa");
}

TEST(TokenizerTest, SentencePieceIsolatedSpacerAndJoinerAnnotate) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::None;
  options.joiner_annotate = true;
  options.preserve_placeholders = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/wmtende.model")));
  test_tok(tokenizer, "a crumpled sofa", "a cru ￭mpl ￭ed sofa");
  test_tok(tokenizer, "a ｟ph｠crumpled sofa", "a ｟ph｠ ￭ cru ￭mpl ￭ed sofa");
  test_tok(tokenizer, "｟ph｠,", "｟ph｠ ￭ ,");
}

TEST(TokenizerTest, SentencePieceAggressiveIsolatedSpacerAndJoinerAnnotate) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  Tokenizer tokenizer(options, std::make_shared<SentencePiece>(get_data("sp-models/wmtende.model")));
  test_tok(tokenizer, "depending on its temperature.", "depending on its temperature ￭.");
}

TEST(TokenizerTest, TokenInterface) {
  Tokenizer::Options options;
  options.mode = Tokenizer::Mode::Aggressive;
  options.joiner_annotate = true;
  options.case_markup = true;
  Tokenizer tokenizer(options);
  const std::string text = "Hello world!";
  std::vector<Token> tokens;
  tokenizer.tokenize(text, tokens);
  EXPECT_EQ(tokens[0].surface, "hello");
  EXPECT_EQ(tokens[1].surface, "world");
  EXPECT_EQ(tokens[2].surface, "!");
  EXPECT_EQ(tokenizer.detokenize(tokens), text);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  assert(argc == 2);
  data_dir = argv[1];
  return RUN_ALL_TESTS();
}
