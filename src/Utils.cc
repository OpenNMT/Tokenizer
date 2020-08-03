#include "onmt/Utils.h"

#include "onmt/Tokenizer.h"

namespace onmt
{

  bool starts_with(const std::string& str, const std::string& prefix)
  {
    return (str.length() >= prefix.length() && str.compare(0, prefix.length(), prefix) == 0);
  }

  bool ends_with(const std::string& str, const std::string& suffix)
  {
    return (str.length() >= suffix.length()
            && str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0);
  }

  bool is_placeholder(const std::string& str)
  {
    size_t ph_begin = str.find(Tokenizer::ph_marker_open);
    if (ph_begin == std::string::npos)
      return false;
    size_t min_ph_end = ph_begin + Tokenizer::ph_marker_open.length() + 1;
    return str.find(Tokenizer::ph_marker_close, min_ph_end) != std::string::npos;
  }

}
