#include "onmt/SubwordEncoder.h"

#include <fstream>
#include <stdexcept>

namespace onmt
{

  void SubwordEncoder::load_vocabulary(const std::string& path, int frequency_threshold)
  {
    std::ifstream in(path);
    if (!in.is_open())
      throw std::invalid_argument("Unable to open vocabulary file `" + path + "'");

    std::vector<std::string> vocab;
    std::string line;
    while (std::getline(in, line))
    {
      size_t sep = line.find(' ');
      if (sep == std::string::npos && frequency_threshold <= 1)
        vocab.emplace_back(std::move(line));
      else if (sep != std::string::npos)
      {
        int frequency = std::stoi(line.substr(sep + 1));
        if (frequency >= frequency_threshold)
          vocab.emplace_back(line.substr(0, sep));
      }
    }

    set_vocabulary(vocab);
  }

  void SubwordEncoder::set_vocabulary(const std::vector<std::string>&)
  {
    return;
  }

  void SubwordEncoder::reset_vocabulary()
  {
    return;
  }

  std::vector<AnnotatedToken> SubwordEncoder::encode_and_annotate(const AnnotatedToken& token) const
  {
    std::vector<std::string> encoded = encode(token.str());
    std::vector<AnnotatedToken> tokens;

    for (size_t j = 0; j < encoded.size(); ++j)
    {
      tokens.emplace_back(encoded[j]);
      if (j + 1 < encoded.size())
        tokens.back().join_right();
    }

    propagate_token_properties(token, tokens);
    return tokens;
  }

  void SubwordEncoder::propagate_token_properties(const AnnotatedToken& token,
                                                  std::vector<AnnotatedToken>& tokens)
  {
    if (token.is_joined_left())
    {
      tokens.front().join_left();
      if (token.should_preserve())
        tokens.front().preserve();
    }
    if (token.is_joined_right())
    {
      tokens.back().join_right();
      if (token.should_preserve())
        tokens.back().preserve();
    }

    if (token.has_case())
    {
      if (tokens.size() == 1 || token.get_case() == CaseModifier::Type::Capitalized)
        tokens.front().set_case(token.get_case());
      else
      {
        tokens.front().set_case_region_begin(token.get_case());
        tokens.back().set_case_region_end(token.get_case());
      }
    }
  }

}
