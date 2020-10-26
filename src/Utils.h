#pragma once

#include <string>
#include <vector>

namespace onmt
{

  bool starts_with(const std::string& str, const std::string& prefix);
  bool ends_with(const std::string& str, const std::string& suffix);

  std::vector<std::string> split_string(const std::string& str, const std::string& separator);

  bool is_placeholder(const std::string& str);

  void set_random_generator_seed(const unsigned int seed);
  unsigned int get_random_generator_seed();

  std::string int_to_hex(int i, int width = 4);

}
