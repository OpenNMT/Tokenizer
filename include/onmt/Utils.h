#pragma once

#include <string>

namespace onmt
{

  bool starts_with(const std::string& str, const std::string& prefix);
  bool ends_with(const std::string& str, const std::string& suffix);

  bool is_placeholder(const std::string& str);

}
