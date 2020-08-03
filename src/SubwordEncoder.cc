#include "onmt/SubwordEncoder.h"

#include <fstream>
#include <stdexcept>

#include "Casing.h"

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

  std::vector<Token> SubwordEncoder::encode_and_annotate(const Token& token) const
  {
    std::vector<std::string> encoded = encode(token.surface);
    std::vector<Token> tokens;

    for (size_t j = 0; j < encoded.size(); ++j)
    {
      tokens.emplace_back(encoded[j]);
      if (j + 1 < encoded.size())
        tokens.back().join_right = true;
    }

    propagate_token_properties(token, tokens);
    return tokens;
  }

  std::vector<Token> SubwordEncoder::encode_and_annotate(const std::vector<Token>& tokens) const
  {
    std::vector<Token> segments;
    segments.reserve(tokens.size() * 2);

    for (const auto& token : tokens)
    {
      if (token.is_placeholder()) {
        segments.push_back(token);
        continue;
      }

      std::vector<Token> sub_segments = encode_and_annotate(token);
      segments.insert(segments.end(),
                      std::make_move_iterator(sub_segments.begin()),
                      std::make_move_iterator(sub_segments.end()));
    }

    return segments;
  }

  void SubwordEncoder::propagate_token_properties(const Token& token, std::vector<Token>& tokens)
  {
    if (token.join_left)
    {
      tokens.front().join_left = true;
      if (token.preserve)
        tokens.front().preserve = true;
    }
    if (token.join_right)
    {
      tokens.back().join_right = true;
      if (token.preserve)
        tokens.back().preserve = true;
    }

    if (token.casing != Casing::None)
    {
      for (size_t i = 0; i < tokens.size(); ++i)
      {
        auto casing = token.casing;
        if (casing == Casing::Capitalized && i > 0)
          casing = Casing::Lowercase;
        else if (casing == Casing::Mixed)
          casing = lowercase_token(tokens[i].surface).second;
        tokens[i].casing = casing;
      }
    }

    if (tokens.size() > 1)
    {
      tokens.front().type = TokenType::LeadingSubword;
      for (size_t i = 1; i < tokens.size(); ++i)
        tokens[i].type = TokenType::TrailingSubword;
    }

    if (token.has_features())
    {
      for (auto& sub_token : tokens)
        sub_token.features = token.features;
    }
  }

}
