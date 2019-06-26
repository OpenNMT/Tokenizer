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

  bool AnnotatedToken::begin_case_region() const
  {
    return _begin_case_region != CaseModifier::Type::None;
  }

  bool AnnotatedToken::end_case_region() const
  {
    return _end_case_region != CaseModifier::Type::None;
  }

  bool AnnotatedToken::has_case() const
  {
    return _case != CaseModifier::Type::None;
  }

  void AnnotatedToken::set_case(CaseModifier::Type type)
  {
    _case = type;
  }

  void AnnotatedToken::set_case_region_begin(CaseModifier::Type type)
  {
    _begin_case_region = type;
  }

  void AnnotatedToken::set_case_region_end(CaseModifier::Type type)
  {
    _end_case_region = type;
  }

  CaseModifier::Type AnnotatedToken::get_case() const
  {
    return _case;
  }

  CaseModifier::Type AnnotatedToken::get_case_region_begin() const
  {
    return _begin_case_region;
  }

  CaseModifier::Type AnnotatedToken::get_case_region_end() const
  {
    return _end_case_region;
  }

  void AnnotatedToken::set_features(const std::vector<std::string>& features)
  {
    _features = features;
  }

  void AnnotatedToken::insert_feature(const std::string& feature)
  {
    _features.push_back(feature);
  }

  bool AnnotatedToken::has_features() const
  {
    return !_features.empty();
  }

  const std::vector<std::string>& AnnotatedToken::features() const
  {
    return _features;
  }

  void AnnotatedToken::set_index(size_t index)
  {
    _index = index;
  }

  size_t AnnotatedToken::get_index() const
  {
    return _index;
  }

}
