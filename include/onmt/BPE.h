#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordEncoder.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT BPE: public SubwordEncoder
  {
  public:
    BPE(const std::string& model_path, const float dropout = 0);
    BPE(const std::string& model_path, const std::string& joiner, const float dropout = 0);

    std::vector<std::string> encode(const std::string& str) const override;

    void set_vocabulary(const std::vector<std::string>& vocabulary) override;
    void reset_vocabulary() override;

    void init_bpe_vocab(const std::string& vocab_path, int bpe_vocab_threshold);
    void set_joiner(std::string joiner)
    {
      _joiner = joiner;
    }

    void set_dropout(const float dropout)
    {
      _dropout = dropout;
    }

  private:
    std::string _end_of_word;
    std::string _begin_of_word;
    bool _prefix;
    bool _suffix;
    bool _case_insensitive;
    std::pair<int, int> _version;

    std::string _joiner;
    float _dropout;

    std::unordered_map<std::string, int> _codes;
    std::unordered_map<std::string, std::pair<std::string, std::string> > _codes_reverse;
    std::unordered_set<std::string> _bpe_vocab;

    void load_model(const std::string& model_path);

    int get_score(const std::string& gram1, const std::string& gram2) const;
    void apply_merges(std::vector<std::string>& chars) const;

    void check_vocab_and_split(const std::vector<std::string> & orig, std::vector<std::string> & out) const;
    void recursive_split(const std::string & segment, std::vector<std::string> & out, bool finalflag = false) const;

    void recursive_split_left(const std::string & segment, std::vector<std::string> & out) const;
    void recursive_split_right(const std::string & segment, std::vector<std::string> & out, bool finalflag) const;
  };

}
