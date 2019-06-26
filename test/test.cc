#include <gtest/gtest.h>

#include <onmt/Tokenizer.h>
#include <onmt/SpaceTokenizer.h>
#include <onmt/Alphabet.h>

using namespace onmt;

static std::string data_dir;

static std::string get_data(const std::string& path) {
  return data_dir + "/" + path;
}

static void test_tok(ITokenizer& tokenizer,
                     const std::string& in,
                     const std::string& expected,
                     bool detokenize = false) {
  auto joined_tokens = tokenizer.tokenize(in);
  EXPECT_EQ(joined_tokens, expected);
  if (detokenize) {
    auto detok = tokenizer.detokenize(joined_tokens);
    EXPECT_EQ(detok, in);
  }
}

static void test_tok(ITokenizer& tokenizer,
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

static void test_detok(ITokenizer& tokenizer, const std::string& in, const std::string& expected) {
  std::vector<std::string> tokens;
  onmt::SpaceTokenizer::get_instance().tokenize(in, tokens);
  auto detok = tokenizer.detokenize(tokens);
  EXPECT_EQ(detok, expected);
}

static void test_tok_alphabet(ITokenizer& tokenizer,
                              const std::string& in,
                              const std::string& expected,
                              const std::unordered_map<std::string, size_t>& expected_alphabets) {
  std::vector<std::string> words;
  std::vector<std::vector<std::string> > features;
  std::unordered_map<std::string, size_t> alphabets;

  tokenizer.tokenize(in, words, features, alphabets);

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

static void test_tok_and_detok(ITokenizer& tokenizer,
                               const std::string& in,
                               const std::string& expected) {
  return test_tok(tokenizer, in, expected, true);
}

TEST(TokenizerTest, DetokenizeWithRanges) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
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
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
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
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
  Ranges ranges;
  tokenizer.detokenize({"｟a｠￭", "b", "￭｟c｠"}, ranges, true);
  // Result: ｟a｠b｟c｠
  ASSERT_EQ(ranges.size(), 3);
  EXPECT_EQ(ranges[0], (std::pair<size_t, size_t>(0, 6)));
  EXPECT_EQ(ranges[1], (std::pair<size_t, size_t>(7, 7)));
  EXPECT_EQ(ranges[2], (std::pair<size_t, size_t>(8, 14)));
}

TEST(TokenizerTest, BasicConservative) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
  test_tok(tokenizer,
           "Your Hardware-Enablement Stack (HWE) is supported until April 2019.",
           "Your Hardware-Enablement Stack ( HWE ) is supported until April 2019 .");
}
TEST(TokenizerTest, ConservativeEmpty) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
  test_tok(tokenizer, "", "");
}

TEST(TokenizerTest, None) {
  Tokenizer tokenizer(Tokenizer::Mode::None);
  test_tok(tokenizer, "Hello World!", "Hello World!");
}

TEST(TokenizerTest, NoneWithPlaceholders1) {
  Tokenizer tokenizer(Tokenizer::Mode::None, Tokenizer::Flags::JoinerAnnotate);
  test_tok(tokenizer, "Hello:｟World｠!", "Hello:￭ ｟World｠￭ !");
}

TEST(TokenizerTest, NoneWithPlaceholders2) {
  Tokenizer tokenizer(Tokenizer::Mode::None, Tokenizer::Flags::JoinerAnnotate |
                      Tokenizer::Flags::PreservePlaceholders);
  test_tok(tokenizer, "Hello:｟World｠!", "Hello:￭ ｟World｠ ￭ !");
}

TEST(TokenizerTest, BasicSpace) {
  Tokenizer tokenizer(Tokenizer::Mode::Space);
  test_tok(tokenizer,
           "49th meeting Social and human rights questions: human rights [14 (g)]",
           "49th meeting Social and human rights questions: human rights [14 (g)]");
}
TEST(TokenizerTest, SpaceEmpty) {
  Tokenizer tokenizer(Tokenizer::Mode::Space);
  test_tok(tokenizer, "", "");
}
TEST(TokenizerTest, SpaceSingle) {
  Tokenizer tokenizer(Tokenizer::Mode::Space);
  test_tok(tokenizer, "Hello", "Hello");
}
TEST(TokenizerTest, SpaceLeading) {
  Tokenizer tokenizer(Tokenizer::Mode::Space);
  test_tok(tokenizer, " Hello", "Hello");
}
TEST(TokenizerTest, SpaceTrailing) {
  Tokenizer tokenizer(Tokenizer::Mode::Space);
  test_tok(tokenizer, "Hello ", "Hello");
}
TEST(TokenizerTest, SpaceDuplicated) {
  Tokenizer tokenizer(Tokenizer::Mode::Space);
  test_tok(tokenizer, "  Hello   World ", "Hello World");
}

TEST(TokenizerTest, BasicJoiner) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::JoinerAnnotate);
  test_tok_and_detok(tokenizer,
                     "Isn't it so-greatly working?",
                     "Isn ￭'￭ t it so ￭-￭ greatly working ￭?");
  test_tok_and_detok(tokenizer, "MP3", "MP ￭3");
  test_tok_and_detok(tokenizer, "A380", "A ￭380");
  test_tok_and_detok(tokenizer, "$1", "$￭ 1");
}

TEST(TokenizerTest, BasicSpaceWithFeatures) {
  Tokenizer tokenizer(Tokenizer::Mode::Space, Tokenizer::Flags::CaseFeature);
  test_tok(tokenizer,
           "49th meeting Social and human rights questions: human rights [14 (g)]",
           "49th￨L meeting￨L social￨C and￨L human￨L rights￨L questions:￨L human￨L rights￨L [14￨N (g)]￨L");
}

TEST(TokenizerTest, ProtectedSequence) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::JoinerAnnotate);
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠￭ km");
  test_tok_and_detok(tokenizer, "A｟380｠", "A ￭｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(tokenizer, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(tokenizer, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(tokenizer, "｟US$｠23", "｟US$｠￭ 23");
  test_tok_and_detok(tokenizer, "1｟ABCD｠0", "1 ￭｟ABCD｠￭ 0");
}

TEST(TokenizerTest, PreserveProtectedSequence) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::PreservePlaceholders);
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠ ￭ km");
  test_tok_and_detok(tokenizer, "A｟380｠", "A ￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠ ￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(tokenizer, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(tokenizer, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(tokenizer, "｟US$｠23", "｟US$｠ ￭ 23");
  test_tok_and_detok(tokenizer, "1｟ABCD｠0", "1 ￭ ｟ABCD｠ ￭ 0");
}

TEST(TokenizerTest, PreserveProtectedSequenceSpacerAnnotate) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::SpacerAnnotate | Tokenizer::Flags::PreservePlaceholders);
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠ km");
  test_tok_and_detok(tokenizer, "A ｟380｠", "A ▁ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠ ｟380｠", "｟1,023｠ ▁ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ .");
  test_tok_and_detok(tokenizer, "｟1023｠ .", "｟1023｠ ▁.");
}

TEST(TokenizerTest, ProtectedSequenceAggressive) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::JoinerAnnotate);
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠￭ km");
  test_tok_and_detok(tokenizer, "A｟380｠", "A ￭｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(tokenizer, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(tokenizer, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(tokenizer, "｟US$｠23", "｟US$｠￭ 23");
  test_tok_and_detok(tokenizer, "1｟ABCD｠0", "1 ￭｟ABCD｠￭ 0");
}

TEST(TokenizerTest, ProtectedSequenceJoinerNew) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::JoinerNew);
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠ ￭ km");
  test_tok_and_detok(tokenizer, "A｟380｠", "A ￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠ ￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ ￭ .");
}

TEST(TokenizerTest, Substitutes) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
  test_tok(tokenizer,
           "test￭ protect￨, ：, ▁, and ％ or ＃...",
           "test ■ protect │ , : , _ , and % or # . . .");
  test_tok(tokenizer, "｟tag：value with spaces｠", "｟tag：value％0020with％0020spaces｠");
}

TEST(TokenizerTest, NoSubstitution) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::NoSubstitution);
  test_tok(tokenizer,
           "test￭ protect￨, ：, ▁, and ％ or ＃...",
           "test ￭ protect ￨ , ： , ▁ , and ％ or ＃ . . .");
  test_tok(tokenizer, "｟tag：value with spaces｠", "｟tag：value with spaces｠");
}

TEST(TokenizerTest, CombiningMark) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::JoinerAnnotate);
  test_tok_and_detok(tokenizer,
                     "वर्तमान लिपि (स्क्रिप्ट) खो जाएगी।",
                     "वर्तमान लिपि (￭ स्क्रिप्ट ￭) खो जाएगी ￭।");
}

TEST(TokenizerTest, MarkOnSpace) {
  Tokenizer tokenizer_joiner(Tokenizer::Mode::Conservative, Tokenizer::Flags::JoinerAnnotate);
  test_tok_and_detok(tokenizer_joiner,
                     "b ̇c",
                     "b ￭％0020̇￭ c");
  Tokenizer tokenizer_spacer(Tokenizer::Mode::Conservative, Tokenizer::Flags::SpacerAnnotate);
  test_tok_and_detok(tokenizer_spacer,
                     "b ̇c",
                     "b ％0020̇ c");
}

TEST(TokenizerTest, MarkOnSpaceNoSubstitution) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::NoSubstitution);
  test_tok(tokenizer, "angles ၧ1 and ၧ2", {"angles", "￭ ၧ￭", "1", "and", "￭ ၧ￭", "2"}, true);
}

TEST(TokenizerTest, CaseFeature) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::CaseFeature | Tokenizer::Flags::JoinerAnnotate);
  // Note: in C literal strings, \ is escaped by another \.
  test_tok(tokenizer,
           "test \\\\\\\\a Capitalized lowercased UPPERCASÉ miXêd - cyrillic-Б",
           "test￨L \\￨N ￭\\￨N ￭\\￨N ￭\\￭￨N a￨L capitalized￨C lowercased￨L uppercasé￨U mixêd￨M -￨N cyrillic-б￨M");
}

TEST(TokenizerTest, CaseMarkupWithJoiners) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::CaseMarkup | Tokenizer::Flags::JoinerAnnotate);
  test_tok_and_detok(tokenizer,
                     "Hello world!", "｟mrk_case_modifier_C｠ hello world ￭!");
  test_tok_and_detok(tokenizer,
                     "Hello WORLD!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ world ｟mrk_end_case_region_U｠ ￭!");
  test_tok_and_detok(tokenizer,
                     "Hello WOrld!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ wo￭ ｟mrk_end_case_region_U｠ rld ￭!");
  test_tok_and_detok(tokenizer,
                     "hello woRld!", "hello wo￭ ｟mrk_case_modifier_C｠ rld ￭!");
  test_tok_and_detok(tokenizer,
                     "hello woRlD!", "hello wo￭ ｟mrk_case_modifier_C｠ rl￭ ｟mrk_case_modifier_C｠ d ￭!");
}

TEST(TokenizerTest, CaseMarkupWithSpacers) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::CaseMarkup | Tokenizer::Flags::SpacerAnnotate);
  test_tok_and_detok(tokenizer,
                     "Hello world!", "｟mrk_case_modifier_C｠ hello ▁world !");
  test_tok_and_detok(tokenizer,
                     "Hello WORLD!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ ▁world ｟mrk_end_case_region_U｠ !");
  test_tok_and_detok(tokenizer,
                     "Hello WOrld!", "｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ ▁wo ｟mrk_end_case_region_U｠ rld !");
  test_tok_and_detok(tokenizer,
                     "hello woRld!", "hello ▁wo ｟mrk_case_modifier_C｠ rld !");
}

TEST(TokenizerTest, CaseMarkupWithBPE) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::CaseMarkup | Tokenizer::Flags::JoinerAnnotate,
                      get_data("bpe-models/codes_suffix_case_insensitive.fr"));
  test_tok_and_detok(tokenizer,
                     "Bonjour monde", "｟mrk_case_modifier_C｠ bon￭ j￭ our mon￭ de");
  test_tok_and_detok(tokenizer,
                     "BONJOUR MONDE", "｟mrk_begin_case_region_U｠ bon￭ j￭ our ｟mrk_end_case_region_U｠ ｟mrk_begin_case_region_U｠ mon￭ de ｟mrk_end_case_region_U｠");
  test_tok_and_detok(tokenizer,
                     "BONJOUR monde", "｟mrk_begin_case_region_U｠ bon￭ j￭ our ｟mrk_end_case_region_U｠ mon￭ de");
}

TEST(TokenizerTest, CaseMarkupDetokMissingModifiedToken) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::CaseMarkup);
  test_detok(tokenizer, "hello ｟mrk_case_modifier_C｠", "hello");
  test_detok(tokenizer, "｟mrk_case_modifier_C｠ ｟mrk_case_modifier_C｠ hello", "Hello");
  test_detok(tokenizer, "｟mrk_case_modifier_C｠ ｟mrk_begin_case_region_U｠ hello ｟mrk_end_case_region_U｠", "HELLO");
}

TEST(TokenizerTest, CaseMarkupDetokMissingRegionMarker) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::CaseMarkup);
  test_detok(tokenizer, "｟mrk_begin_case_region_U｠ hello", "HELLO");
  test_detok(tokenizer,
             "｟mrk_begin_case_region_U｠ hello ｟mrk_case_modifier_C｠ world", "HELLO World");
  test_detok(tokenizer,
             "｟mrk_end_case_region_U｠ hello ｟mrk_case_modifier_C｠ world", "hello World");
}

TEST(TokenizerTest, CaseMarkupDetokNestedMarkers) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::CaseMarkup);
  test_detok(tokenizer,
             "｟mrk_begin_case_region_U｠ ｟mrk_case_modifier_C｠ hello world ｟mrk_end_case_region_U｠", "Hello WORLD");
  test_detok(tokenizer,
             "｟mrk_begin_case_region_U｠ hello ｟mrk_case_modifier_C｠ ｟mrk_end_case_region_U｠ world", "HELLO world");
}

TEST(TokenizerTest, SegmentCase) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::CaseFeature | Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::SegmentCase);
  test_tok_and_detok(tokenizer, "WiFi", "wi￭￨C fi￨C");
}

TEST(TokenizerTest, SegmentNumbers) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::SegmentNumbers);
  test_tok_and_detok(tokenizer,
                     "1984 mille neuf cent quatrevingt-quatre",
                     "1￭ 9￭ 8￭ 4 mille neuf cent quatrevingt ￭-￭ quatre");
}

TEST(TokenizerTest, GetAlphabet) {
  for (const auto& a : onmt::alphabet_ranges) {
    const auto& range = a.first;
    const auto& id = a.second;

    EXPECT_EQ(static_cast<int>(id), onmt::get_alphabet_id(range.first));
    EXPECT_EQ(static_cast<int>(id), onmt::get_alphabet_id(range.second));
  }
}

TEST(TokenizerTest, SegmentAlphabet) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::JoinerAnnotate);
  tokenizer.add_alphabet_to_segment("Han");
  test_tok_and_detok(tokenizer, "rawБ", "rawБ");
  test_tok(tokenizer,
           "有入聲嘅唐話往往有陽入對轉，即係入聲韻尾同鼻音韻尾可以轉化。比如粵語嘅「抌」（dam）「揼」（dap），意思接近，意味微妙，區別在於-m同-p嘅轉換。",
           "有￭ 入￭ 聲￭ 嘅￭ 唐￭ 話￭ 往￭ 往￭ 有￭ 陽￭ 入￭ 對￭ 轉 ￭，￭ 即￭ 係￭ 入￭ 聲￭ 韻￭ 尾￭ 同￭ 鼻￭ 音￭ 韻￭ 尾￭ 可￭ 以￭ 轉￭ 化 ￭。￭ 比￭ 如￭ 粵￭ 語￭ 嘅 ￭「￭ 抌 ￭」 ￭（￭ dam ￭） ￭「￭ 揼 ￭」 ￭（￭ dap ￭） ￭，￭ 意￭ 思￭ 接￭ 近 ￭，￭ 意￭ 味￭ 微￭ 妙 ￭，￭ 區￭ 別￭ 在￭ 於-m同-p嘅￭ 轉￭ 換 ￭。");
}

TEST(TokenizerTest, SegmentAlphabetChange) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::SegmentAlphabetChange);
  test_tok(tokenizer, "rawБ", "raw Б");
}

TEST(TokenizerTest, PreserveSegmentedNumbers) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive,
                      Tokenizer::Flags::SegmentNumbers
                      | Tokenizer::Flags::JoinerAnnotate
                      | Tokenizer::Flags::PreserveSegmentedTokens);
  test_tok_and_detok(tokenizer, "1234", "1 ￭ 2 ￭ 3 ￭ 4");
}

TEST(TokenizerTest, PreserveSegmentAlphabetChange) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::SegmentAlphabetChange
                      | Tokenizer::Flags::JoinerAnnotate
                      | Tokenizer::Flags::PreserveSegmentedTokens);
  test_tok_and_detok(tokenizer, "rawБ", "raw ￭ Б");
}

TEST(TokenizerTest, PreserveSegmentAlphabet) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::JoinerAnnotate
                      | Tokenizer::Flags::SegmentAlphabetChange
                      | Tokenizer::Flags::PreserveSegmentedTokens);
  tokenizer.add_alphabet_to_segment("Han");
  test_tok_and_detok(tokenizer, "測試abc", "測 ￭ 試 ￭ abc");
}

TEST(TokenizerTest, PreserveSegmentCase) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::SegmentCase
                      | Tokenizer::Flags::JoinerAnnotate
                      | Tokenizer::Flags::PreserveSegmentedTokens);
  test_tok_and_detok(tokenizer, "WiFi", "Wi ￭ Fi");
}

TEST(TokenizerTest, PreserveSegmentCaseBPE) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative,
                      Tokenizer::Flags::SegmentCase
                      | Tokenizer::Flags::JoinerAnnotate
                      | Tokenizer::Flags::PreserveSegmentedTokens,
                      get_data("bpe-models/testcode.v0.1"));
  test_tok_and_detok(tokenizer, "iF", "i ￭ F");
}

TEST(TokenizerTest, BPEBasic) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::JoinerAnnotate,
                      get_data("bpe-models/testcode.v0.1"));
  test_tok_and_detok(tokenizer,
                     "abcdimprovement联合国",
                     "a￭ b￭ c￭ d￭ impr￭ ovemen￭ t￭ 联合￭ 国");
}

TEST(TokenizerTest, BPEModePrefix) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::None,
                      get_data("bpe-models/codes_prefix.fr"));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S e u lement seulement il v ais n on se u lement seulement n on à V er d un");
}

TEST(TokenizerTest, BPEModeNofix) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::None,
                      get_data("bpe-models/codes_nofix.fr"));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S e u lement seulement il v ais n on seulement seulement n on à V er d un");
}

TEST(TokenizerTest, BPEModeBoth) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::None,
                      get_data("bpe-models/codes_bothfix.fr"));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S eu lement seulement il va is n on s eu lement seu l emen t n on à V er du n");
}

TEST(TokenizerTest, BPECaseInsensitive) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::None,
                      get_data("bpe-models/codes_suffix_case_insensitive.fr"));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "Seulement seulement il va is n on seulement seu l em ent n on à Ver d un");
}

TEST(TokenizerTest, SpacerAnnotate) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::SpacerAnnotate);
  test_tok_and_detok(tokenizer,
                     "Isn't it so-greatly working?",
                     "Isn ' t ▁it ▁so - greatly ▁working ?");
  test_tok_and_detok(tokenizer, "MP3", "MP 3");
}

TEST(TokenizerTest, SpacerNew) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive,
                      Tokenizer::Flags::SpacerAnnotate | Tokenizer::Flags::SpacerNew);
  test_tok_and_detok(tokenizer,
                     "Isn't it so-greatly working?",
                     "Isn ' t ▁ it ▁ so - greatly ▁ working ?");
}

TEST(TokenizerTest, Alphabets) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::SegmentAlphabetChange);
  std::unordered_map<std::string, size_t> lat_cyrillic_alphabets;
  lat_cyrillic_alphabets["Latin"] = 3;
  lat_cyrillic_alphabets["Cyrillic"] = 1;
  test_tok_alphabet(tokenizer, "rawБ", "raw Б", lat_cyrillic_alphabets);

  std::unordered_map<std::string, size_t> han2;
  han2["Han"] = 2;
  test_tok_alphabet(tokenizer, "有 入", "有 入", han2);
}

TEST(TokenizerTest, ArabicAlphabet) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
  std::unordered_map<std::string, size_t> alphabets {
    {"Arabic", 5}
  };
  test_tok_alphabet(tokenizer, "مرحبا", "مرحبا", alphabets);
}

TEST(TokenizerTest, NonbreakableSpace) {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative);
  test_tok(tokenizer, "a b", "a b");
}

TEST(TokenizerTest, CharMode) {
  Tokenizer tokenizer(Tokenizer::Mode::Char);
  test_tok(tokenizer, "  Hello   World 123.", "H e l l o W o r l d 1 2 3 .");
}

TEST(TokenizerTest, CharModeSpacer) {
  Tokenizer tokenizer(Tokenizer::Mode::Char, Tokenizer::Flags::SpacerAnnotate);
  test_tok(tokenizer, "  Hello   World 123.", "H e l l o ▁W o r l d ▁1 2 3 .");
}

TEST(TokenizerTest, CharModeSpacerNew) {
  Tokenizer tokenizer(Tokenizer::Mode::Char, Tokenizer::Flags::SpacerAnnotate | Tokenizer::Flags::SpacerNew);
  test_tok(tokenizer, "  Hello   World 123.", "H e l l o ▁ W o r l d ▁ 1 2 3 .");
}

#ifdef WITH_SP

#  include <onmt/SentencePiece.h>

TEST(TokenizerTest, SentencePiece) {
  Tokenizer tokenizer(Tokenizer::Mode::None, Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/sp.model"));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The ▁two ▁shows , ▁called ▁De si re ▁and ▁S e c re t s , ▁will ▁be ▁one - hour ▁prime - time ▁shows .");
}

TEST(TokenizerTest, SentencePieceObject) {
  SentencePiece sp(get_data("sp-models/sp.model"));
  Tokenizer tokenizer(Tokenizer::Mode::None, &sp);
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The ▁two ▁shows , ▁called ▁De si re ▁and ▁S e c re t s , ▁will ▁be ▁one - hour ▁prime - time ▁shows .");
}

TEST(TokenizerTest, SentencePieceWithJoinersAndPh) {
  Tokenizer tokenizer(Tokenizer::Mode::None,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/sp.model"));
  test_tok_and_detok(tokenizer,
                     "The two shows, called ｟Desire｠ and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called ｟Desire｠ and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
  test_tok_and_detok(tokenizer,
                     "The two shows, called｟Desire｠and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called￭ ｟Desire｠￭ and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
}

TEST(TokenizerTest, SentencePieceWithJoinersAndPh_preserve) {
  Tokenizer tokenizer(Tokenizer::Mode::None,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::SentencePieceModel|
                      Tokenizer::Flags::PreservePlaceholders,
                      get_data("sp-models/sp.model"));
  test_tok_and_detok(tokenizer,
                     "The two shows, called｟Desire｠and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called￭ ｟Desire｠ ￭ and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
}

#ifdef SP_HAS_SAMPLE_ENCODE
TEST(TokenizerTest, SentencePieceSubwordRegularization) {
  Tokenizer tokenizer(get_data("sp-models/sp_regularization.model"), 1, 0.1);
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "▁The ▁ two ▁show s , ▁call ed ▁De si re ▁ and ▁Sec re t s , ▁w ill ▁be ▁one - h our ▁ pri me - t im e ▁show s .");
}
#endif

TEST(TokenizerTest, SentencePieceAlt) {
  Tokenizer tokenizer(Tokenizer::Mode::None, Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/wmtende.model"));
  test_tok_and_detok(tokenizer,
                     "Bamford is appealing the sentence and has been granted bail of 50,000 baht.",
                     "▁Ba m ford ▁is ▁appealing ▁the ▁sentence ▁and ▁has ▁been ▁granted ▁bail ▁of ▁ 50,000 ▁ba ht .");
}

TEST(TokenizerTest, SentencePieceWithJoiners) {
  Tokenizer tokenizer(Tokenizer::Mode::None,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/sp.model"));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The two shows ￭, called De ￭si ￭re and S ￭e ￭c ￭re ￭t ￭s ￭, will be one ￭- ￭hour prime ￭- ￭time shows ￭.");
}

TEST(TokenizerTest, AggressiveWithSentencePiece) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive, Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/wmtende.model"));
  test_tok(tokenizer,
           "Bamford is appealing the sentence and has been granted bail of 50,000 baht.",
           "Ba m ford is appealing the sentence and has been granted bail of 50 , 000 ba ht .");
}

TEST(TokenizerTest, AggressiveWithSentencePieceAndSpacers) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive,
                      Tokenizer::Flags::SpacerAnnotate | Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/sp.model"));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The ▁t wo ▁s how s , ▁called ▁D es ir e ▁and ▁Se c re t s , ▁will ▁be ▁one - hour ▁p rime - time ▁s how s .");
}

TEST(TokenizerTest, AggressiveWithSentencePieceAndJoiners) {
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::SentencePieceModel,
                      get_data("sp-models/sp.model"));
  test_tok_and_detok(tokenizer,
                     "The two shows, called Desire and Secrets, will be one-hour prime-time shows.",
                     "The t ￭wo s ￭how ￭s ￭, called D ￭es ￭ir ￭e and Se ￭c ￭re ￭t ￭s ￭, will be one ￭-￭ hour p ￭rime ￭-￭ time s ￭how ￭s ￭.");
}

#else

TEST(TokenizerTest, NoSentencePieceSupport) {
  ASSERT_THROW(Tokenizer(Tokenizer::Mode::None, Tokenizer::Flags::SentencePieceModel,
                         get_data("sp-models/sp.model")),
               std::runtime_error);
}

#endif

TEST(TokenizerTest, WithoutVocabulary) {
  Tokenizer tokenizer(Tokenizer::Mode::Space,
                      Tokenizer::Flags::JoinerAnnotate,
                      get_data("bpe-models/bpe_code.v0.2"),
                      "@@");
  test_tok(tokenizer,
           "Oliver Grün , welle",
           "Oliver Grün , welle");
}

TEST(TokenizerTest, WithVocabulary) {
  Tokenizer tokenizer(Tokenizer::Mode::Space,
                      Tokenizer::Flags::JoinerAnnotate,
                      get_data("bpe-models/bpe_code.v0.2"),
                      "@@",
                      get_data("bpe-models/vocab.en"),
                      50);
  test_tok(tokenizer,
           "Oliver Grün , welle",
           "Oliver Gr@@ ü@@ n , wel@@ le");
}

TEST(TokenizerTest, AnnotatedTokenInterface)
{
  Tokenizer tokenizer(Tokenizer::Mode::Aggressive,
                      Tokenizer::Flags::JoinerAnnotate | Tokenizer::Flags::CaseMarkup);
  const std::string text = "Hello world!";
  std::vector<AnnotatedToken> tokens;
  tokenizer.tokenize(text, tokens);
  EXPECT_EQ(tokens[0].str(), "hello");
  EXPECT_EQ(tokens[1].str(), "world");
  EXPECT_EQ(tokens[2].str(), "!");
  EXPECT_EQ(tokenizer.detokenize(tokens), text);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  assert(argc == 2);
  data_dir = argv[1];
  return RUN_ALL_TESTS();
}
