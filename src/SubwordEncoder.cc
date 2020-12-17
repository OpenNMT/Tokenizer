#include "onmt/SubwordEncoder.h"

#include <fstream>
#include <stdexcept>

#include "Casing.h"

namespace onmt
{

  void SubwordEncoder::update_tokenization_options(Tokenizer::Options&) const
  {
  }

  void SubwordEncoder::load_vocabulary(const std::string& path,
                                       int frequency_threshold,
                                       const Tokenizer::Options* options)
  {
    std::ifstream in(path);
    if (!in)
      throw std::invalid_argument("Unable to open vocabulary file `" + path + "'");

    std::vector<std::string> vocab;
    std::string line;
    std::string token;
    int frequency;
    while (std::getline(in, line))
    {
      size_t sep = line.find(' ');
      if (sep == std::string::npos)
        sep = line.find('\t');

      if (sep == std::string::npos)
      {
        token = std::move(line);
        frequency = 1;
      }
      else
      {
        token = line.substr(0, sep);
        frequency = std::stoi(line.substr(sep + 1));
      }

      if (frequency >= frequency_threshold)
        vocab.emplace_back(std::move(token));
    }

    set_vocabulary(vocab, options);
  }

  void SubwordEncoder::set_vocabulary(const std::vector<std::string>&, const Tokenizer::Options*)
  {
  }

  void SubwordEncoder::reset_vocabulary()
  {
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
    tokens.front().join_left = token.join_left;
    tokens.back().join_right = token.join_right;

    tokens.front().preserve = token.join_left && token.preserve;
    tokens.back().preserve = token.join_right && token.preserve;

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
