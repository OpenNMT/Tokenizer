#include "onmt/Vocab.h"

#include <algorithm>
#include <numeric>

#include "Utils.h"

namespace onmt
{
  const std::string Vocab::unk_token = "<unk>";

  Vocab::Vocab(const std::vector<std::string>& special_tokens)
  {
    for (const auto& token : special_tokens)
      add_token(token);
    for (auto& frequency : _frequencies)
      frequency = std::numeric_limits<size_t>::max();
  }

  Vocab Vocab::load_from_file(std::istream& is) {
    Vocab vocab;
    std::string token;
    while (std::getline(is, token))
      vocab.add_token(std::move(token));
    return vocab;
  }

  Vocab load_from_tokens(const std::vector<std::string>& tokens) {
    Vocab vocab;
    for (const auto& token : tokens)
      vocab.add_token(token);
    return vocab;
  }

  void Vocab::add_token(std::string token)
  {
    const size_t index = _ids_to_tokens.size();
    const auto pair = _tokens_to_ids.emplace(std::move(token), index);
    const auto& entry = *pair.first;
    const bool inserted = pair.second;

    if (inserted)
    {
      _ids_to_tokens.emplace_back(entry.first);
      _frequencies.emplace_back(1);
    }
    else if (_frequencies[entry.second] < std::numeric_limits<size_t>::max())
    {
      _frequencies[entry.second]++;
    }
  }

  void Vocab::add_from_text(const std::string& text, const Tokenizer* tokenizer)
  {
    std::vector<std::string> tokens;
    if (tokenizer)
      tokenizer->tokenize(text, tokens);
    else
      tokens = split_string(text, " ");
    for (auto& token : tokens)
      add_token(std::move(token));
  }

  void Vocab::add_from_stream(std::istream& is, const Tokenizer* tokenizer)
  {
    std::string line;
    while (std::getline(is, line))
      add_from_text(line, tokenizer);
  }

  void Vocab::resize(size_t maximum_size, size_t minimum_frequency)
  {
    if (maximum_size == 0 && minimum_frequency == 1)
      return;

    std::vector<size_t> ids(size());
    std::iota(ids.begin(), ids.end(), size_t(0));
    std::stable_sort(ids.begin(), ids.end(), [this](size_t i1, size_t i2) {
      return _frequencies[i1] > _frequencies[i2];
    });

    while (!ids.empty() && _frequencies[ids.back()] < minimum_frequency)
      ids.pop_back();

    if (maximum_size > 0 && ids.size() > maximum_size)
      ids.resize(maximum_size);

    const size_t new_size = ids.size();

    std::unordered_map<std::string, size_t> new_tokens_to_ids;
    std::vector<std::string> new_ids_to_tokens;
    std::vector<size_t> new_frequencies;
    new_tokens_to_ids.reserve(new_size);
    new_ids_to_tokens.reserve(new_size);
    new_frequencies.reserve(new_size);

    for (size_t new_id = 0; new_id < new_size; ++new_id)
    {
      const size_t old_id = ids[new_id];
      new_frequencies.emplace_back(_frequencies[old_id]);
      new_ids_to_tokens.emplace_back(std::move(_ids_to_tokens[old_id]));
      new_tokens_to_ids.emplace(new_ids_to_tokens.back(), new_id);
    }

    _tokens_to_ids = std::move(new_tokens_to_ids);
    _ids_to_tokens = std::move(new_ids_to_tokens);
    _frequencies = std::move(new_frequencies);
  }

}
