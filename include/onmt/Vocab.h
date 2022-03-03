#pragma once

#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Tokenizer.h"

namespace onmt
{

  class Vocab
  {
  public:
    static const std::string unk_token;

    Vocab() = default;
    Vocab(const std::vector<std::string>& special_tokens);

    static Vocab load_from_file(std::istream& is);
    static Vocab load_from_tokens(const std::vector<std::string>& tokens);

    size_t lookup(const std::string& token) const
    {
      auto it = _tokens_to_ids.find(token);
      if (it == _tokens_to_ids.end())
      {
        it = _tokens_to_ids.find(unk_token);
        if (it == _tokens_to_ids.end())
          return size();
      }
      return it->second;
    }

    const std::string& lookup(size_t index) const
    {
      if (index >= size())
        return unk_token;
      return _ids_to_tokens[index];
    }

    bool contains(const std::string& token) const
    {
      return _tokens_to_ids.find(token) != _tokens_to_ids.end();
    }

    size_t size() const
    {
      return _ids_to_tokens.size();
    }

    const std::unordered_map<std::string, size_t>& tokens_to_ids() const
    {
      return _tokens_to_ids;
    }

    const std::vector<std::string>& ids_to_tokens() const
    {
      return _ids_to_tokens;
    }

    void add_token(std::string token);
    void add_from_text(const std::string& text, const Tokenizer* tokenizer = nullptr);
    void add_from_stream(std::istream& is, const Tokenizer* tokenizer = nullptr);
    void resize(size_t maximum_size = 0, size_t minimum_frequency = 1);

  private:
    std::unordered_map<std::string, size_t> _tokens_to_ids;
    std::vector<std::string> _ids_to_tokens;
    std::vector<size_t> _frequencies;
  };

}
