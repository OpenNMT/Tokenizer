/*
Use byte pair encoding (BPE) to learn a variable-length encoding of the vocabulary in a text.
Unlike the original BPE, it does not compress the plain text, but can be used to reduce the vocabulary
of a text to a configurable number of symbols, with only a small increase in the number of tokens.

Reference:
Rico Sennrich, Barry Haddow and Alexandra Birch (2016). Neural Machine Translation of Rare Words with Subword Units.
Proceedings of the 54th Annual Meeting of the Association for Computational Linguistics (ACL 2016). Berlin, Germany.

The code is converted from bpe_learn.py (https://github.com/rsennrich/subword-nmts) - version #6e67561
*/

#include "onmt/BPELearner.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

#include "onmt/unicode/Unicode.h"

namespace onmt
{
  typedef std::pair<std::string, std::string> bigram;
  typedef std::vector<std::string> sequence;

  BPELearner::BPELearner(bool verbose,
                         int symbols,
                         int min_frequency,
                         bool dict_input,
                         bool total_symbols)
    : SubwordLearner(verbose, new Tokenizer(Tokenizer::Mode::Space))
    , _symbols(symbols)
    , _min_frequency(min_frequency)
    , _dict_input(dict_input)
    , _total_symbols(total_symbols)
  {
  }

  void BPELearner::load_from_dictionary(std::istream& is)
  {
    std::string line;
    while (std::getline(is, line))
    {
      if (line.empty())
        continue;
      size_t p = line.find(" ");
      if (p == std::string::npos || line.find(" ", p + 1) != std::string::npos)
        throw std::runtime_error("Failed reading vocabulary file");
      _vocab[line.substr(0, p)] += std::stoi(line.substr(p + 1));
    }
  }

  void BPELearner::ingest_token_impl(const std::string& token)
  {
    _vocab[token]++;
  }

  void BPELearner::ingest(std::istream& is, const Tokenizer* tokenizer)
  {
    if (_dict_input)
      load_from_dictionary(is);
    else
      SubwordLearner::ingest(is, tokenizer);
  }

  struct Change {
    Change(int j_,
           const sequence& word_,
           sequence&& old_word_,
           int freq_)
      : j(j_)
      , word(word_)
      , old_word(old_word_)
      , freq(freq_) {
    }

    int j;
    const sequence& word;
    sequence old_word;
    int freq;
  };

  struct pair_hash {
    template <typename T1, typename T2>
    std::size_t operator () (const std::pair<T1, T2>& pair) const {
      const std::size_t h1 = std::hash<T1>()(pair.first);
      const std::size_t h2 = std::hash<T2>()(pair.second);
      return h1 ^ h2;
    }
  };

  using bigram_collection = std::unordered_set<bigram, pair_hash>;

  static const bigram*
  get_bigram(bigram_collection& collection,
             const std::string& a,
             const std::string& b) {
    return &(*collection.emplace(a, b).first);
  }

  static void
  update_pair_statistics(bigram_collection& collection,
                         const bigram* pair,
                         const std::vector<Change>& changed,
                         std::unordered_map<const bigram*, int>& stats,
                         std::unordered_map<const bigram*, std::unordered_map<int, int>>& indices) {
    /* Minimally update the indices and frequency of symbol pairs

    if we merge a pair of symbols, only pairs that overlap with occurrences
    of this pair are affected, and need to be updated.
    */

    stats[pair] = 0;
    indices[pair] = std::unordered_map<int, int>();
    const std::string &first = pair->first;
    const std::string &second = pair->second;

    std::string new_pair = first + second;

    for(const auto& change : changed) {
      const int j = change.j;
      const sequence& word = change.word;
      const sequence& old_word = change.old_word;
      const int freq = change.freq;

      // find all instances of pair, and update frequency/indices around it
      size_t i = 0;
      while (true) {
        // find first symbol
        auto it = std::find(old_word.begin() + i, old_word.end(), first);
        if (it == old_word.end())
          break;
        i = it - old_word.begin();
        // if first symbol is followed by second symbol, we've found an occurrence of pair (old_word[i:i+2])
        if (i < old_word.size()-1 && old_word[i+1] == second) {
          // assuming a symbol sequence "A B C", if "B C" is merged, reduce the frequency of "A B"
          if (i > 0) {
            const bigram* prev = get_bigram(collection, old_word[i-1], old_word[i]);
            stats[prev] -= freq;
            indices[prev][j] -= 1;
          }
          if (i < old_word.size()-2) {
            // assuming a symbol sequence "A B C B", if "B C" is merged, reduce the frequency of "C B".
            // however, skip this if the sequence is A B C B C, because the frequency of "C B" will be reduced by the previous code block
            if (old_word[i+2] != first || i >= old_word.size()-3 || old_word[i+3] != second) {
              const bigram* nex = get_bigram(collection, old_word[i+1], old_word[i+2]);
              stats[nex] -= freq;
              indices[nex][j] -= 1;
            }
          }
          i += 2;
        }
        else
          i += 1;
      }

      i = 0;
      while (true) {
        // find new pair
        auto it = std::find(word.begin() + i, word.end(), new_pair);
        if (it == word.end())
          break;
        i = it - word.begin();
        // assuming a symbol sequence "A BC D", if "B C" is merged, increase the frequency of "A BC"
        if (i) {
          const bigram* prev = get_bigram(collection, word[i-1], word[i]);
          stats[prev] += freq;
          indices[prev][j] += 1;
        }
        // assuming a symbol sequence "A BC B", if "B C" is merged, increase the frequency of "BC B"
        // however, if the sequence is A BC BC, skip this step because the count of "BC BC" will be incremented by the previous code block
        if (i < word.size()-1 && word[i+1] != new_pair) {
          const bigram* nex = get_bigram(collection, word[i], word[i+1]);
          stats[nex] += freq;
          indices[nex][j] += 1;
        }
        i += 1;
      }
    }
  }

  static void
  get_pair_statistics(bigram_collection& collection,
                      const std::vector<std::pair<int, sequence>>& sorted_vocab,
                      std::unordered_map<const bigram*, int>& stats,
                      std::unordered_map<const bigram*, std::unordered_map<int, int>>& indices) {
    /* Count frequency of all symbol pairs, and create index */
    
    std::unordered_set<std::string> uniq_char_internal;
    std::unordered_set<std::string> uniq_char_final;
    int max = 0;
    for(size_t i = 0; i < sorted_vocab.size(); i++) {
      int freq = sorted_vocab[i].first;
      const sequence &word = sorted_vocab[i].second;
      for(size_t j = 1; j < word.size(); j++) {
        const bigram* pair = get_bigram(collection, word[j-1], word[j]);
        stats[pair] += freq;
        if (stats[pair] > max)
          max = stats[pair];
        indices[pair][i] += 1;
        uniq_char_internal.insert(word[j-1]);
      }
      uniq_char_final.insert(word.back());
    }
  }

  static std::vector<Change>
  replace_pair(const bigram* pair,
               std::vector<std::pair<int, sequence>>& sorted_vocab,
               std::unordered_map<const bigram*, std::unordered_map<int, int>>& indices) {
    /* Replace all occurrences of a symbol pair ('A', 'B') with a new symbol 'AB' */
    const std::string &A = pair->first;
    const std::string &B = pair->second;

    const auto& pair_indices = indices[pair];
    std::vector<Change> changes;
    changes.reserve(pair_indices.size());
    for (const auto& index : pair_indices) {
      if (index.second < 1)
        continue;
      const int j = index.first;
      sequence &word = sorted_vocab[j].second;
      int freq = sorted_vocab[j].first;

      sequence wordcopy = word;
      for(size_t h=0; h < word.size()-1; h++)
        if (word[h] == A && word[h+1] == B) {
          word[h] += B;
          word.erase(word.begin()+h+1);
        }
      changes.emplace_back(j, word, std::move(wordcopy), freq);
    }

    return changes;
  }

  static void prune_stats(std::unordered_map<const bigram*, int>& stats,
                          std::unordered_map<const bigram*, int>& big_stats,
                          float threshold) {
    /* Prune statistics dict for efficiency of max()

    The frequency of a symbol pair never increases, so pruning is generally safe
    (until we the most frequent pair is less frequent than a pair we previously pruned)
    big_stats keeps full statistics for when we need to access pruned items
    */
    std::unordered_map<const bigram*, int> pruned_stats;
    for (auto& stat : stats) {
      const int freq = stat.second;
      if (freq < threshold) {
        const bigram* item = stat.first;
        if (freq < 0)
          big_stats[item] += freq;
        else
          big_stats[item] = freq;
      } else {
        pruned_stats.emplace(std::move(stat));
      }
    }
    stats = std::move(pruned_stats);
  }

  static std::vector<std::pair<int, sequence>>
  get_inv_char_frequency(const std::unordered_map<std::string, int>& vocab) {
    /* convert vocab into character sequence+</w> and sort by inv frequency */
    std::multimap<int, sequence> char_vocab;
    for (const auto& pair : vocab) {
      const std::string& token = pair.first;
      const int frequency = pair.second;
      sequence chars;
      unicode::explode_utf8_with_marks(token, chars);
      chars.back().append("</w>");
      char_vocab.emplace(-frequency, std::move(chars));
    }

    std::vector<std::pair<int, sequence>> sorted_vocab;
    sorted_vocab.reserve(char_vocab.size());
    for (auto& pair : char_vocab) {
      const int inv_frequency = pair.first;
      sequence& chars = pair.second;
      sorted_vocab.emplace_back(-inv_frequency, std::move(chars));
    }

    return sorted_vocab;
  }

  static std::pair<const bigram*, int>
  get_most_frequent(const std::unordered_map<const bigram*, int>& stats) {
    // The comparison on the bigram is to have the same priority as previous
    // versions of this code that iterates on a std::map.
    return *std::max_element(stats.begin(), stats.end(),
                             [](const std::pair<const bigram*, int>& a,
                                const std::pair<const bigram*, int>& b) {
                               return (a.second < b.second
                                       || (a.second == b.second && *a.first > *b.first));
                             });
  }

  void BPELearner::learn(std::ostream &os, const char *description, bool verbose) {
    verbose = verbose || _verbose;
    os << "#version: 0.2\n";    
    if (description) {
      std::string desc = std::string("# ") + description;
      size_t p = desc.find("\n");
      while (p != std::string::npos) {
        desc.replace(p, 1, "\n# ");
        p = desc.find("\n", p+1);
      }
      os << desc << std::endl;
    }

    std::vector<std::pair<int, sequence>> sorted_vocab = get_inv_char_frequency(_vocab);
    bigram_collection collection;
    std::unordered_map<const bigram*, int> stats;
    std::unordered_map<const bigram*, std::unordered_map<int, int>> indices;
    get_pair_statistics(collection, sorted_vocab, stats, indices);

    std::unordered_map<const bigram*, int> big_stats(stats);

    if (_total_symbols) {
      std::unordered_set<std::string> uniq_char_internal;
      std::unordered_set<std::string> uniq_char_final;
      for(size_t i = 0; i < sorted_vocab.size(); i++) {
        const sequence &word = sorted_vocab[i].second;
        for(size_t j = 1; j < word.size(); j++) {
          uniq_char_internal.insert(word[j-1]);
        }
        uniq_char_final.insert(word.back());
      }
      std::cerr << "Number of word-internal characters: " << uniq_char_internal.size() << std::endl;
      std::cerr << "Number of word-final characters: " << uniq_char_final.size() << std::endl;
      std::cerr << "Reducing number of merge operations by " << 
                   uniq_char_internal.size() + uniq_char_final.size() << std::endl;
      _symbols -= uniq_char_internal.size() + uniq_char_final.size();
    }

    const int max = stats.empty() ? -1: get_most_frequent(stats).second;

    float threshold = max / 10.;
    for(int i = 0; i < _symbols; i++) {
      const bigram* most_frequent = nullptr;
      int max_freq = -1;
      if (!stats.empty())
        std::tie(most_frequent, max_freq) = get_most_frequent(stats);

      if (stats.empty() || (i && max_freq < threshold)) {
        prune_stats(stats, big_stats, threshold);
        stats = big_stats;
        std::tie(most_frequent, max_freq) = get_most_frequent(stats);
        // threshold is inspired by Zipfian assumption, but should only affect speed
        threshold = max_freq * i/(i+10000.0);
        prune_stats(stats, big_stats, threshold);
      }

      if (max_freq < _min_frequency) {
        std::cerr << "no pair has frequency >= " << _min_frequency << ". Stopping\n";
        break;
      }
      if (verbose)
        std::cerr << "pair " << i << ": " << most_frequent->first << " " << most_frequent->second <<
                     " -> " << most_frequent->first << most_frequent->second <<
                     " (frequency " << max_freq << ")\n";
      
      os << most_frequent->first << " " << most_frequent->second << "\n";

      const std::vector<Change> changes = replace_pair(most_frequent, sorted_vocab, indices);
      update_pair_statistics(collection, most_frequent, changes, stats, indices);
      stats[most_frequent] = 0;
      if (i % 100 == 0)
        prune_stats(stats, big_stats, threshold);
    }
  }

}
