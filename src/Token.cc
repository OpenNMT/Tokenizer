#include "onmt/Token.h"

#include "onmt/unicode/Unicode.h"
#include "Utils.h"

namespace onmt
{

  bool Token::is_placeholder() const {
    return ::onmt::is_placeholder(surface);
  }

  size_t Token::unicode_length() const {
    return unicode::utf8len(surface);
  }

}
