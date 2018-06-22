#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "onmt/SubwordEncoder.h"

namespace onmt
{

  class BPE: public SubwordEncoder
  {
  public:
    BPE(const std::string& model_path);

    std::vector<std::string> encode(const std::string& str) const override;

  private:
    std::string _end_of_word;
    std::string _begin_of_word;
    bool _prefix;
    bool _suffix;
    bool _case_insensitive;

    std::unordered_map<std::string, int> _codes;

    int get_min_pair_index(const std::vector<std::string>& chars) const;

  };

}
