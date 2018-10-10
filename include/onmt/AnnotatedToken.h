#pragma once

#include <string>

#include "onmt/CaseModifier.h"

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
    std::string& get_str();

    void join_right();
    void join_left();
    void spacer();
    void preserve();

    bool is_joined_left() const;
    bool is_joined_right() const;
    bool is_spacer() const;
    bool should_preserve() const;

    bool begin_case_region() const;
    bool end_case_region() const;
    bool has_case() const;

    void set_case(CaseModifier::Type type);
    void set_case_region_begin(CaseModifier::Type type);
    void set_case_region_end(CaseModifier::Type type);
    CaseModifier::Type get_case() const;
    CaseModifier::Type get_case_region_begin() const;
    CaseModifier::Type get_case_region_end() const;

  private:
    std::string _str;
    CaseModifier::Type _case = CaseModifier::Type::None;
    CaseModifier::Type _begin_case_region = CaseModifier::Type::None;
    CaseModifier::Type _end_case_region = CaseModifier::Type::None;
    bool _join_left = false;
    bool _join_right = false;
    bool _spacer = false;
    bool _preserved = false;
  };

}
