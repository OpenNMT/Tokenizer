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
#include <fstream>
#include <limits>
#include <list>

#include "onmt/unicode/Unicode.h"
#include "onmt/Tokenizer.h"

namespace onmt
{
  typedef std::pair<std::string, std::string> bigram;
  typedef std::vector<std::string> sequence;

  std::string _S(const sequence &s) {
    std::string t;
    for(const auto& w: s)
      t += ", u'" + w + "'";
    return "(" + t.substr(2) + ")";
  }

  BPELearner::BPELearner(bool verbose,
                         int symbols, int min_frequency, bool dict_input, bool total_symbols):
              SubwordLearner(verbose),
              _symbols(symbols), _min_frequency(min_frequency),
              _dict_input(dict_input), _total_symbols(total_symbols) {
  }

  void BPELearner::ingest(std::istream &is, Tokenizer *pTok) {
    if (!pTok)
      pTok = this->_pTokDefault;
    pTok->unset_annotate();
    // get_vocabulary - builds vocabulary from file or dictionary
    while (!is.eof()) {
      std::string line;
      std::getline(is, line);
      if (line.length()) {
        if (_dict_input) {
          size_t p = line.find(" ");
          if (p == std::string::npos || 
              line.find(" ", p+1) != std::string::npos) {
            throw std::runtime_error("Failed reading vocabulary file");
          }
          _vocab[line.substr(0, p)] += std::stoi(line.substr(p+1));
        } else {
          sequence words;
          std::vector<sequence> features;
          pTok->tokenize(line, words, features);
          for(const auto& w: words) {
            if (!Tokenizer::is_placeholder(w))
              _vocab[w]++;
          }
        }
      }
    }
  }

  class change {
  public:
    change(int j, const sequence &new_word,
          const sequence &word, int freq):
      _j(j), _new_word(new_word), _word(word), _freq(freq) {}
    int _j;
    sequence _new_word;
    sequence _word;
    int _freq;
  };

  static void update_pair_statistics(const bigram &pair,
                                     std::list<change> &changed,
                                     std::map<bigram, int> &stats,
                                     std::map<bigram, std::map<int, int> > &indices) {
    /* Minimally update the indices and frequency of symbol pairs

    if we merge a pair of symbols, only pairs that overlap with occurrences
    of this pair are affected, and need to be updated.
    */

    stats[pair] = 0;
    indices[pair].clear();
    const std::string &first = pair.first;
    const std::string &second = pair.second;

    std::string new_pair = first + second;

    for(auto &change: changed) {
      int j = change._j;
      sequence word = change._new_word;
      sequence old_word = change._word;
      int freq = change._freq;

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
            bigram prev(old_word[i-1], old_word[i]);
            stats[prev] -= freq;
            indices[prev][j] -= 1;
          }
          if (i < old_word.size()-2) {
            // assuming a symbol sequence "A B C B", if "B C" is merged, reduce the frequency of "C B".
            // however, skip this if the sequence is A B C B C, because the frequency of "C B" will be reduced by the previous code block
            if (old_word[i+2] != first || i >= old_word.size()-3 || old_word[i+3] != second) {
              bigram nex(old_word[i+1], old_word[i+2]);
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
          bigram prev(word[i-1], word[i]);
          stats[prev] += freq;
          indices[prev][j] += 1;
        }
        // assuming a symbol sequence "A BC B", if "B C" is merged, increase the frequency of "BC B"
        // however, if the sequence is A BC BC, skip this step because the count of "BC BC" will be incremented by the previous code block
        if (i < word.size()-1 && word[i+1] != new_pair) {
          bigram nex(word[i], word[i+1]);
          stats[nex] += freq;
          indices[nex][j] += 1;
        }
        i += 1;
      }
    }
  }

  static void get_pair_statistics(const std::vector<std::pair<int, sequence > > &sorted_vocab,
                                  std::map<bigram, int> &stats,
                                  std::map<bigram, std::map<int, int> > &indices) {
    /* Count frequency of all symbol pairs, and create index */
    
    std::set<std::string> uniq_char_internal;
    std::set<std::string> uniq_char_final;
    int max = 0;
    for(size_t i = 0; i < sorted_vocab.size(); i++) {
      int freq = sorted_vocab[i].first;
      const sequence &word = sorted_vocab[i].second;
      for(size_t j = 1; j < word.size(); j++) {
        bigram pair = std::make_pair(word[j-1], word[j]);
        stats[pair] += freq;
        if (stats[pair] > max)
          max = stats[pair];
        indices[pair][i] += 1;
        uniq_char_internal.insert(word[j-1]);
      }
      uniq_char_final.insert(word.back());
    }
  }

  static std::list<change> replace_pair(
            const bigram pair,
            std::vector<std::pair<int, sequence > > &sorted_vocab,
            std::map<bigram, std::map<int, int> > &indices) {
    /* Replace all occurrences of a symbol pair ('A', 'B') with a new symbol 'AB' */
    const std::string &A = pair.first;
    const std::string &B = pair.second;
    
    std::list<change> changes;
    for(auto it = indices[pair].begin(); 
        it != indices[pair].end(); it++) {
      if (it->second < 1)
        continue;
      int j = it->first;
      sequence &word = sorted_vocab[j].second;
      int freq = sorted_vocab[j].first;

      sequence wordcopy = word;
      for(size_t h=0; h < word.size()-1; h++)
        if (word[h] == A && word[h+1] == B) {
          word[h] += B;
          word.erase(word.begin()+h+1);
        }
      changes.push_back(change(j, word, wordcopy, freq));
    }

    return changes;
  }

  static void prune_stats(std::map<bigram, int> &stats,
                          std::map<bigram, int> &big_stats,
                          float threshold) {
    /* Prune statistics dict for efficiency of max()

    The frequency of a symbol pair never increases, so pruning is generally safe
    (until we the most frequent pair is less frequent than a pair we previously pruned)
    big_stats keeps full statistics for when we need to access pruned items
    */
    for(auto it = stats.begin(); it != stats.end();) {
      auto itnext = it;
      itnext++;
      bigram item = it->first;
      int freq = it->second;
      if (freq < threshold) {
        stats.erase(it);
        if (freq < 0)
          big_stats[item] += freq;
        else
          big_stats[item] = freq;
      }
      it = itnext;
    }
  }

  void BPELearner::learn(std::ostream &os, const char *description) {
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

    /* convert vocab into character sequence+</w> and sort by inv frequency */
    std::multimap<int, sequence > charvocab;
    for(auto it = _vocab.begin(); it != _vocab.end(); it++) {
      sequence chars;
      std::vector<unicode::code_point_t> code_points;
      unicode::explode_utf8(it->first, chars, code_points);
      chars.back().append("</w>");
      charvocab.insert(std::make_pair(-it->second, chars));
    }
    std::vector<std::pair<int, sequence > > sorted_vocab;
    sorted_vocab.reserve(charvocab.size());
    for(auto it = charvocab.begin(); it != charvocab.end(); it++)
      sorted_vocab.push_back(std::make_pair(-it->first, it->second));

    std::map<bigram, int> stats;
    std::map<bigram, std::map<int, int> > indices;
    get_pair_statistics(sorted_vocab, stats, indices);

    std::map<bigram, int> big_stats(stats);

    if (_total_symbols) {
      std::set<std::string> uniq_char_internal;
      std::set<std::string> uniq_char_final;
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

    std::set<std::pair<int, bigram > > invstats;
    for(auto it = stats.begin(); it != stats.end(); it++)
      invstats.insert(std::make_pair(-it->second, it->first));

    int max = -1;
    for(auto k = stats.begin(); k != stats.end(); k++) 
      if (k->second > max) {
        max = k->second;
      }

    float threshold = max / 10.;
    for(int i = 0; i < _symbols; i++) {
      bigram most_frequent;
      if (stats.size() > 0) {
        int frequency = -1;
        for(auto k = stats.begin(); k != stats.end(); k++) 
          if (k->second > frequency) {
            most_frequent = k->first;
            frequency = k->second;
          }
      }

      if (invstats.size() == 0 || (i && stats[most_frequent] < threshold)) {
        prune_stats(stats, big_stats, threshold);
        stats = big_stats;
        int frequency = -1;
        for(auto k = stats.begin(); k != stats.end(); k++) 
          if (k->second > frequency) {
            most_frequent = k->first;
            frequency = k->second;
          }
        // threshold is inspired by Zipfian assumption, but should only affect speed
        threshold = stats[most_frequent] * i/(i+10000.0);
        prune_stats(stats, big_stats, threshold);
      }

      if (stats[most_frequent] < _min_frequency) {
        std::cerr << "no pair has frequency >= " << _min_frequency << ". Stopping\n";
        break;
      }
      if (_verbose)
        std::cerr << "pair " << i << ": " << most_frequent.first << " " << most_frequent.second <<
                     " -> " << most_frequent.first << most_frequent.second <<
                     " (frequency " << stats[most_frequent] << ")\n";
      
      os << most_frequent.first << " " << most_frequent.second << "\n";

      std::list<change> changes = replace_pair(most_frequent, sorted_vocab, indices);
      update_pair_statistics(most_frequent, changes, stats, indices);
      stats[most_frequent] = 0;
      if (i % 100 == 0)
        prune_stats(stats, big_stats, threshold);
    }
  }

}
