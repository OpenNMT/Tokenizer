#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "onmt/SubwordEncoder.h"

namespace onmt
{

  class BPE: public SubwordEncoder
  {
  public:
    BPE(const std::string& model_path);

    std::vector<std::string> encode(const std::string& str) const override;

    void init_bpe_vocab(std::string path, int bpe_vocab_threshold);
    void set_joiner(std::string joiner)
    {
      _joiner = joiner;
    }

  private:
    std::string _end_of_word;
    std::string _begin_of_word;
    bool _prefix;
    bool _suffix;
    bool _case_insensitive;
    bool _compat_03;

    std::string _joiner;

    std::unordered_map<std::string, int> _codes;
    std::unordered_map<std::string, std::pair<std::string, std::string> > _codes_reverse;
    std::unordered_set<std::string> _bpe_vocab;

    int get_min_pair_index(const std::vector<std::string>& chars) const;

    void check_vocab_and_split(std::vector<std::string> & orig, std::vector<std::string> & out) const;
    void recursive_split(std::string & segment, std::vector<std::string> & out, bool finalflag = false) const;

    void recursive_split_left(std::string & segment, std::vector<std::string> & out) const;
    void recursive_split_right(std::string & segment, std::vector<std::string> & out, bool finalflag) const;
  };

}
