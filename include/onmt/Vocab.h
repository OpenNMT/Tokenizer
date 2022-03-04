#pragma once

#include <istream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "Tokenizer.h"

#include "onmt/opennmttokenizer_export.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT Vocab
  {
  public:
    static const std::string unk_token;

    Vocab() = default;
    Vocab(const std::vector<std::string>& special_tokens);

    size_t lookup(const std::string& token) const
    {
      const auto it = _tokens_to_ids.find(token);
      if (it == _tokens_to_ids.end())
        return get_default_id();
      return it->second;
    }

    const std::string& lookup(size_t id) const
    {
      if (id >= size())
        return unk_token;
      return _ids_to_tokens[id];
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

    void set_default_id(size_t id)
    {
      _default_id = id;
    }

    size_t get_default_id() const
    {
      if (_default_id < std::numeric_limits<size_t>::max())
        return _default_id;
      const auto it = _tokens_to_ids.find(unk_token);
      if (it == _tokens_to_ids.end())
        return size();
      return it->second;
    }

  private:
    std::unordered_map<std::string, size_t> _tokens_to_ids;
    std::vector<std::string> _ids_to_tokens;
    std::vector<size_t> _frequencies;
    size_t _default_id = std::numeric_limits<size_t>::max();
  };

}
