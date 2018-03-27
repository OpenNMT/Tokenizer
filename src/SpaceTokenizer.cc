#include "onmt/SpaceTokenizer.h"

#include <sstream>

#include "onmt/unicode/Unicode.h"

namespace onmt
{

  ITokenizer& SpaceTokenizer::get_instance()
  {
    static SpaceTokenizer tokenizer;
    return tokenizer;
  }

  void SpaceTokenizer::tokenize(const std::string& text,
                                std::vector<std::string>& words,
                                std::vector<std::vector<std::string> >& features,
                                std::set<std::string>&) const {
    return tokenize(text, words, features);
  }

  void SpaceTokenizer::tokenize(const std::string& text,
                                std::vector<std::string>& words,
                                std::vector<std::vector<std::string> >& features) const
  {
    std::vector<std::string> chunks = unicode::split_utf8(text, " ");

    for (const auto& chunk: chunks)
    {
      if (chunk.empty())
        continue;

      std::vector<std::string> fields = unicode::split_utf8(chunk, ITokenizer::feature_marker);

      words.push_back(fields[0]);

      for (size_t i = 1; i < fields.size(); ++i)
      {
        if (features.size() < i)
          features.emplace_back(1, fields[i]);
        else
          features[i - 1].push_back(fields[i]);
      }
    }
  }

  std::string SpaceTokenizer::detokenize(const std::vector<std::string>& words,
                                         const std::vector<std::vector<std::string> >& features) const
  {
    std::ostringstream oss;

    for (size_t i = 0; i < words.size(); ++i)
    {
      if (i > 0)
        oss << " ";
      oss << words[i];

      if (!features.empty())
      {
        for (size_t j = 0; j < features.size(); ++j)
          oss << ITokenizer::feature_marker << features[j][i];
      }
    }

    return oss.str();
  }

}
