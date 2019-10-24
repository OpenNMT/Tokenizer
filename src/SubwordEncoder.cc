#include "onmt/SubwordEncoder.h"

#include <fstream>
#include <stdexcept>

namespace onmt
{

  void SubwordEncoder::load_vocabulary(const std::string& path, int frequency_threshold)
  {
    std::ifstream in(path);
    if (!in)
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
      for (size_t i = 0; i < tokens.size(); ++i)
      {
        auto case_type = token.get_case();
        if (case_type == CaseModifier::Type::Capitalized && i > 0)
          case_type = CaseModifier::Type::Lowercase;
        else if (case_type == CaseModifier::Type::Mixed)
          case_type = CaseModifier::extract_case_type(tokens[i].str()).second;
        tokens[i].set_case(case_type);
      }

      if (token.begin_case_region())
      {
        tokens.front().set_case_region_begin(token.get_case());
        tokens.back().set_case_region_end(token.get_case());
      }
    }

    if (token.has_features())
    {
      for (auto& sub_token : tokens)
        sub_token.set_features(token.features());
    }
  }

}
