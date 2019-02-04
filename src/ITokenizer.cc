#include "onmt/ITokenizer.h"

#include "onmt/SpaceTokenizer.h"

namespace onmt
{

  const std::string ITokenizer::feature_marker("￨");

  void ITokenizer::tokenize(const std::string& text,
                            std::vector<std::string>& words,
                            std::vector<std::vector<std::string> >& features,
                            std::unordered_map<std::string, size_t>&) const
  {
    tokenize(text, words, features);
  }

  void ITokenizer::tokenize(const std::string& text, std::vector<std::string>& words) const
  {
    std::vector<std::vector<std::string> > features;
    tokenize(text, words, features);
  }

  std::string ITokenizer::detokenize(const std::vector<std::string>& words) const
  {
    std::vector<std::vector<std::string> > features;
    return detokenize(words, features);
  }

  std::string ITokenizer::tokenize(const std::string& text) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    tokenize(text, words, features);

    return SpaceTokenizer::get_instance().detokenize(words, features);
  }

  std::string ITokenizer::detokenize(const std::string& text) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    SpaceTokenizer::get_instance().tokenize(text, words, features);

    return detokenize(words, features);
  }

  std::string ITokenizer::detokenize(const std::vector<std::string>& words,
                                     Ranges& ranges, bool merge_ranges) const
  {
    std::vector<std::vector<std::string>> features;
    return detokenize(words, features, ranges, merge_ranges);
  }

  std::string ITokenizer::detokenize(const std::vector<std::string>& words,
                                     const std::vector<std::vector<std::string> >& features,
                                     Ranges&, bool) const
  {
    return detokenize(words, features);
  }

}
