#include "Utils.h"

#include <iomanip>
#include <random>
#include <sstream>

#include <sentencepiece_processor.h>

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

  std::vector<std::string> split_string(const std::string& str,
                                        const std::string& separator)
  {
    std::vector<std::string> parts;
    if (str.size() == 0)
      return parts;

    parts.reserve(str.size() / 2);

    size_t offset = 0;
    do {
      const size_t index = str.find(separator, offset);
      if (index == std::string::npos) {
        parts.emplace_back(str, offset);
        break;
      }

      const size_t length = index - offset;
      if (length > 0)
        parts.emplace_back(str, offset, length);
      offset = index + separator.size();
    } while (offset < str.size());

    return parts;
  }

  bool is_placeholder(const std::string& str)
  {
    size_t ph_begin = str.find(Tokenizer::ph_marker_open);
    if (ph_begin == std::string::npos)
      return false;
    size_t min_ph_end = ph_begin + Tokenizer::ph_marker_open.length() + 1;
    return str.find(Tokenizer::ph_marker_close, min_ph_end) != std::string::npos;
  }

  constexpr unsigned int default_seed = static_cast<unsigned int>(-1);
  static unsigned int g_seed = default_seed;

  void set_random_generator_seed(const unsigned int seed)
  {
    g_seed = seed;
    sentencepiece::SetRandomGeneratorSeed(seed);
  }

  unsigned int get_random_generator_seed()
  {
    return g_seed == default_seed ? std::random_device{}() : g_seed;
  }

  std::string int_to_hex(int i, int width)
  {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(width) << std::hex << i;
    return stream.str();
  }

}
