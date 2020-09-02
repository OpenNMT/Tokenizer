#include "onmt/SpaceTokenizer.h"

#include <sstream>

#include "Utils.h"

namespace onmt
{

  ITokenizer& SpaceTokenizer::get_instance()
  {
    static SpaceTokenizer tokenizer;
    return tokenizer;
  }

  void SpaceTokenizer::tokenize(const std::string& text,
                                std::vector<std::string>& words,
                                std::vector<std::vector<std::string> >& features) const
  {
    words.reserve(text.length());

    size_t offset = 0;

    while (true)
    {
      size_t space_pos = text.find(' ', offset);
      if (space_pos == std::string::npos)
      {
        words.emplace_back(text, offset);
        break;
      }
      else if (space_pos != offset)
        words.emplace_back(text, offset, space_pos - offset);
      offset = space_pos + 1;
    }

    if (words[0].find(ITokenizer::feature_marker) != std::string::npos)
    {
      for (auto& word : words)
      {
        std::vector<std::string> fields = split_string(word, ITokenizer::feature_marker);
        word = fields[0];
        for (size_t i = 1; i < fields.size(); ++i)
        {
          if (features.size() < i)
            features.emplace_back(1, fields[i]);
          else
            features[i - 1].emplace_back(std::move(fields[i]));
        }
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
