#include "onmt/AnnotatedToken.h"

namespace onmt
{

  AnnotatedToken::AnnotatedToken(std::string&& str)
    : _str(std::move(str))
  {
  }

  AnnotatedToken::AnnotatedToken(const std::string& str)
    : _str(str)
  {
  }

  void AnnotatedToken::append(const std::string& str)
  {
    _str += str;
  }

  void AnnotatedToken::set(std::string&& str)
  {
    _str = std::move(str);
  }

  void AnnotatedToken::set(const std::string& str)
  {
    _str = str;
  }

  void AnnotatedToken::clear()
  {
    _str.clear();
    _join_right = false;
    _join_left = false;
    _preserved = false;
  }

  const std::string& AnnotatedToken::str() const
  {
    return _str;
  }

  std::string& AnnotatedToken::get_str()
  {
    return _str;
  }

  void AnnotatedToken::join_right()
  {
    _join_right = true;
  }

  void AnnotatedToken::join_left()
  {
    _join_left = true;
  }

  void AnnotatedToken::spacer()
  {
    _spacer = true;
  }

  void AnnotatedToken::preserve()
  {
    _preserved = true;
  }

  bool AnnotatedToken::is_joined_right() const
  {
    return _join_right;
  }

  bool AnnotatedToken::is_joined_left() const
  {
    return _join_left;
  }

  bool AnnotatedToken::is_spacer() const
  {
    return _spacer;
  }

  bool AnnotatedToken::should_preserve() const
  {
    return _preserved;
  }

}
