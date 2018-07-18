#pragma once

#include <string>

namespace onmt
{

  class AnnotatedToken
  {
  public:
    AnnotatedToken() = default;
    AnnotatedToken(std::string&& str);
    AnnotatedToken(const std::string& str);

    void append(const std::string& str);
    void set(std::string&& str);
    void set(const std::string& str);
    void clear();
    const std::string& str() const;

    void join_right();
    void join_left();
    void spacer();

    bool is_joined_left() const;
    bool is_joined_right() const;
    bool is_spacer() const;

  private:
    std::string _str;
    bool _join_left = false;
    bool _join_right = false;
    bool _spacer = false;
  };

}
