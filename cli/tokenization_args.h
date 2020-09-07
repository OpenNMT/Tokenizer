#pragma once

#include <cxxopts.hpp>

#include <onmt/Tokenizer.h>

inline void add_tokenization_options(cxxopts::Options& options)
{
  options.add_options("General tokenization")
    ("m,mode", "Set the tokenization mode (can be: conservative, aggressive, char, space, none)",
     cxxopts::value<std::string>()->default_value("conservative"))
    ("no_substitution", "Do not replace special characters found in the input text",
     cxxopts::value<bool>()->default_value("false"))
    ;

  options.add_options("Reversible tokenization")
    ("j,joiner_annotate", "Mark joints with joiner tokens (mutually exclusive with spacer_annotate)",
     cxxopts::value<bool>()->default_value("false"))
    ("joiner", "Set the joiner token",
     cxxopts::value<std::string>()->default_value(onmt::Tokenizer::joiner_marker))
    ("joiner_new", "Make joiners independent tokens",
     cxxopts::value<bool>()->default_value("false"))
    ("spacer_annotate", "Mark spaces with spacer tokens (mutually exclusive with joiner_annotate)",
     cxxopts::value<bool>()->default_value("false"))
    ("spacer_new", "Make spacers independent tokens",
     cxxopts::value<bool>()->default_value("false"))
    ("preserve_placeholders", "Do not attach joiners or spacers to placeholders",
     cxxopts::value<bool>()->default_value("false"))
    ("preserve_segmented_tokens",
     "Do not attach joiners or spacers to segmented tokens (see segment_* options)",
     cxxopts::value<bool>()->default_value("false"))
    ("support_prior_joiners", "If the input text has joiners, keep them",
     cxxopts::value<bool>()->default_value("false"))
    ;

  options.add_options("Segmentation")
    ("segment_case", "Segment on case change",
     cxxopts::value<bool>()->default_value("false"))
    ("segment_numbers", "Segment numbers into single digits",
     cxxopts::value<bool>()->default_value("false"))
    ("segment_alphabet", "Comma-separated list of scripts on which to segment all letters",
     cxxopts::value<std::vector<std::string>>()->default_value(""))
    ("segment_alphabet_change", "Segment if the script changes between 2 letters",
     cxxopts::value<bool>()->default_value("false"))
    ;

  options.add_options("Case management")
    ("c,case_feature", "Lowercase input text and generate case feature",
     cxxopts::value<bool>()->default_value("false"))
    ("case_markup", "Lowercase input text and inject case markup tokens",
     cxxopts::value<bool>()->default_value("false"))
    ("soft_case_regions", "Allow case invariant tokens to be included in case regions",
     cxxopts::value<bool>()->default_value("false"))
    ;
}

inline int build_tokenization_flags(const cxxopts::ParseResult& args)
{
  int flags = 0;
  if (args["no_substitution"].as<bool>())
    flags |= onmt::Tokenizer::Flags::NoSubstitution;
  if (args["joiner_annotate"].as<bool>())
    flags |= onmt::Tokenizer::Flags::JoinerAnnotate;
  if (args["joiner_new"].as<bool>())
    flags |= onmt::Tokenizer::Flags::JoinerNew;
  if (args["spacer_annotate"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SpacerAnnotate;
  if (args["spacer_new"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SpacerNew;
  if (args["preserve_placeholders"].as<bool>())
    flags |= onmt::Tokenizer::Flags::PreservePlaceholders;
  if (args["preserve_segmented_tokens"].as<bool>())
    flags |= onmt::Tokenizer::Flags::PreserveSegmentedTokens;
  if (args["support_prior_joiners"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SupportPriorJoiners;
  if (args["segment_case"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SegmentCase;
  if (args["segment_numbers"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SegmentNumbers;
  if (args["segment_alphabet_change"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SegmentAlphabetChange;
  if (args["case_feature"].as<bool>())
    flags |= onmt::Tokenizer::Flags::CaseFeature;
  if (args["case_markup"].as<bool>())
    flags |= onmt::Tokenizer::Flags::CaseMarkup;
  if (args["soft_case_regions"].as<bool>())
    flags |= onmt::Tokenizer::Flags::SoftCaseRegions;
  return flags;
}
