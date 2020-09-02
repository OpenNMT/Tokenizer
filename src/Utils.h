#pragma once

#include <string>
#include <vector>

namespace onmt
{

  bool starts_with(const std::string& str, const std::string& prefix);
  bool ends_with(const std::string& str, const std::string& suffix);

  std::vector<std::string> split_string(const std::string& str, const std::string& separator);

  bool is_placeholder(const std::string& str);

}
