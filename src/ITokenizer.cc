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

    std::string output;

    for (size_t i = 0; i < words.size(); ++i)
    {
      if (i > 0)
        output += " ";
      output += words[i];
      for (size_t j = 0; j < features.size(); ++j)
        output += feature_marker + features[j][i];
    }

    return output;
  }

  std::string ITokenizer::detokenize(const std::string& text) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    SpaceTokenizer::get_instance().tokenize(text, words, features);

    return detokenize(words, features);
  }

}
