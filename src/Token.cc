#include "onmt/Token.h"

#include <tuple>

#include "onmt/unicode/Unicode.h"
#include "Casing.h"
#include "Utils.h"

namespace onmt
{

  bool Token::is_placeholder() const {
    return ::onmt::is_placeholder(surface);
  }

  size_t Token::unicode_length() const {
    return unicode::utf8len(surface);
  }

  void Token::lowercase() {
    if (is_placeholder())
      return;
    std::tie(surface, casing) = lowercase_token(surface);
  }

}
