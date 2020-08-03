#pragma once

#include <string>
#include <vector>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/unicode/Unicode.h"
#include "onmt/Casing.h"

namespace onmt
{

  enum class TokenType
  {
    Word,
    LeadingSubword,
    TrailingSubword,
  };

  class OPENNMTTOKENIZER_EXPORT Token
  {
  public:
    std::string surface;
    TokenType type = TokenType::Word;
    Casing casing = Casing::None;
    bool join_left = false;
    bool join_right = false;
    bool spacer = false;
    bool preserve = false;
    std::vector<std::string> features;

    Token() = default;
    Token(std::string str)
      : surface(std::move(str))
    {
    }

    void append(const std::string& str)
    {
      surface += str;
    }

    bool empty() const {
      return surface.empty();
    }

    size_t unicode_length() const {
      return unicode::utf8len(surface);
    }

    void append_feature(std::string feature)
    {
      features.emplace_back(std::move(feature));
    }

    bool has_features() const
    {
      return !features.empty();
    }

    bool operator==(const Token& other) const
    {
      return (surface == other.surface
              && type == other.type
              && casing == other.casing
              && join_left == other.join_left
              && join_right == other.join_right
              && spacer == other.spacer
              && preserve == other.preserve
              && features == other.features);
    }

  };

}
