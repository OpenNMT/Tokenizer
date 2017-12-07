#include <memory>

#include <gtest/gtest.h>

#include <onmt/Tokenizer.h>

static std::string data_dir;

static std::string get_data(const std::string& path) {
  return data_dir + "/" + path;
}

static void test_tok(std::unique_ptr<onmt::ITokenizer>& tokenizer,
                     const std::string& in,
                     const std::string& expected,
                     bool detokenize = false) {
  auto joined_tokens = tokenizer->tokenize(in);
  EXPECT_EQ(expected, joined_tokens);
  if (detokenize) {
    auto detok = tokenizer->detokenize(joined_tokens);
    EXPECT_EQ(in, detok);
  }
}

static void test_tok_and_detok(std::unique_ptr<onmt::ITokenizer>& tokenizer,
                               const std::string& in,
                               const std::string& expected) {
  return test_tok(tokenizer, in, expected, true);
}

TEST(TokenizerTest, BasicConservative) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative));
  test_tok(tokenizer,
           "Your Hardware-Enablement Stack (HWE) is supported until April 2019.",
           "Your Hardware-Enablement Stack ( HWE ) is supported until April 2019 .");
}

TEST(TokenizerTest, BasicSpace) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Space));
  test_tok(tokenizer,
           "49th meeting Social and human rights questions: human rights [14 (g)]",
           "49th meeting Social and human rights questions: human rights [14 (g)]");
}

TEST(TokenizerTest, BasicJoiner) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive, "", false, true));
  test_tok_and_detok(tokenizer,
                     "Isn't it so-greatly working?",
                     "Isn ￭'￭ t it so ￭-￭ greatly working ￭?");
  test_tok_and_detok(tokenizer, "MP3", "MP ￭3");
  test_tok_and_detok(tokenizer, "A380", "A ￭380");
  test_tok_and_detok(tokenizer, "$1", "$￭ 1");
}

TEST(TokenizerTest, BasicSpaceWithFeatures) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Space, "", true));
  test_tok(tokenizer,
           "49th meeting Social and human rights questions: human rights [14 (g)]",
           "49th￨L meeting￨L social￨C and￨L human￨L rights￨L questions:￨L human￨L rights￨L [14￨N (g)]￨L");
}

TEST(TokenizerTest, ProtectedSequence) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative, "", false, true));
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠￭ km");
  test_tok_and_detok(tokenizer, "A｟380｠", "A ￭｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ ￭.");
  test_tok_and_detok(tokenizer, "$｟0.23｠", "$￭ ｟0.23｠");
  test_tok_and_detok(tokenizer, "｟0.23｠$", "｟0.23｠ ￭$");
  test_tok_and_detok(tokenizer, "｟US$｠23", "｟US$｠￭ 23");
  test_tok_and_detok(tokenizer, "1｟ABCD｠0", "1 ￭｟ABCD｠￭ 0");
}

TEST(TokenizerTest, ProtectedSequenceAggressive) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive, "", false, true));
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
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative, "", false, true, true));
  test_tok_and_detok(tokenizer, "｟1,023｠km", "｟1,023｠ ￭ km");
  test_tok_and_detok(tokenizer, "A｟380｠", "A ￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1,023｠｟380｠", "｟1,023｠ ￭ ｟380｠");
  test_tok_and_detok(tokenizer, "｟1023｠.", "｟1023｠ ￭ .");
}

TEST(TokenizerTest, Substitutes) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative));
  test_tok(tokenizer,
           "test￭ protect￨, ：, and ％ or ＃...",
           "test ■ protect │ , : , and % or # . . .");
}

TEST(TokenizerTest, CombiningMark) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative, "", false, true));
  test_tok_and_detok(tokenizer,
                     "वर्तमान लिपि (स्क्रिप्ट) खो जाएगी।",
                     "वर्तमान लिपि (￭ स्क्रिप्ट ￭) खो जाएगी ￭।");
}

TEST(TokenizerTest, CaseFeature) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative, "", true, true));
  // Note: in C literal strings, \ is escaped by another \.
  test_tok(tokenizer,
           "test \\\\\\\\a Capitalized lowercased UPPERCASÉ miXêd - cyrillic-Б",
           "test￨L \\￨N ￭\\￨N ￭\\￨N ￭\\￭￨N a￨L capitalized￨C lowercased￨L uppercasé￨U mixêd￨M -￨N cyrillic-б￨M");
}

TEST(TokenizerTest, SegmentCase) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative, "", true, true, false,
                        onmt::Tokenizer::joiner_marker, false, true));
  test_tok_and_detok(tokenizer, "WiFi", "wi￭￨C fi￨C");
}

TEST(TokenizerTest, SegmentNumbers) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive, "", false, true, false,
                        onmt::Tokenizer::joiner_marker, false, false, true));
  test_tok_and_detok(tokenizer,
                     "1984 mille neuf cent quatrevingt-quatre",
                     "1￭ 9￭ 8￭ 4 mille neuf cent quatrevingt ￭-￭ quatre");
}

TEST(TokenizerTest, BPEBasic) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Conservative,
                        get_data("bpe-models/testcode"), false, true));
  test_tok_and_detok(tokenizer,
                     "abcdimprovement联合国",
                     "a￭ b￭ c￭ d￭ impr￭ ovemen￭ t￭ 联合￭ 国");
}

TEST(TokenizerTest, BPEModePrefix) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive, get_data("bpe-models/codes_prefix.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S e u lement seulement il v ais n on se u lement seulement n on à V er d un");
}

TEST(TokenizerTest, BPEModeNofix) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive, get_data("bpe-models/codes_nofix.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S e u lement seulement il v ais n on seulement seulement n on à V er d un");
}

TEST(TokenizerTest, BPEModeBoth) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive, get_data("bpe-models/codes_bothfix.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "S eu lement seulement il va is n on s eu lement seu l emen t n on à V er du n");
}

TEST(TokenizerTest, BPECaseInsensitive) {
  auto tokenizer = std::unique_ptr<onmt::ITokenizer>(
    new onmt::Tokenizer(onmt::Tokenizer::Mode::Aggressive,
                        get_data("bpe-models/codes_suffix_case_insensitive.fr")));
  test_tok(tokenizer,
           "Seulement seulement il vais nonseulement seulementnon à Verdun",
           "Seulement seulement il va is n on seulement seu l em ent n on à Ver d un");
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  assert(argc == 2);
  data_dir = argv[1];
  return RUN_ALL_TESTS();
}
