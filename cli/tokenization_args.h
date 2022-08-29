#pragma once

#include <cxxopts.hpp>

#include <onmt/Tokenizer.h>

inline void add_tokenization_options(cxxopts::Options& options)
{
  options.add_options("General tokenization")
    ("m,mode", "Set the tokenization mode (can be: conservative, aggressive, char, space, none)",
     cxxopts::value<std::string>()->default_value("conservative"))
    ("lang", "ISO language code of the input",
     cxxopts::value<std::string>()->default_value(""))
    ("no_substitution", "Do not replace special characters found in the input text",
     cxxopts::value<bool>()->default_value("false"))
    ("with_separators", "Include separator characters in the tokenized output",
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

inline onmt::Tokenizer::Options build_tokenization_options(const cxxopts::ParseResult& args)
{
  onmt::Tokenizer::Options options;
  options.mode = onmt::Tokenizer::str_to_mode(args["mode"].as<std::string>());
  options.lang = args["lang"].as<std::string>();
  options.no_substitution = args["no_substitution"].as<bool>();
  options.with_separators = args["with_separators"].as<bool>();
  options.case_feature = args["case_feature"].as<bool>();
  options.case_markup = args["case_markup"].as<bool>();
  options.soft_case_regions = args["soft_case_regions"].as<bool>();
  options.joiner_annotate = args["joiner_annotate"].as<bool>();
  options.joiner_new = args["joiner_new"].as<bool>();
  options.joiner = args["joiner"].as<std::string>();
  options.spacer_annotate = args["spacer_annotate"].as<bool>();
  options.spacer_new = args["spacer_new"].as<bool>();
  options.preserve_placeholders = args["preserve_placeholders"].as<bool>();
  options.preserve_segmented_tokens = args["preserve_segmented_tokens"].as<bool>();
  options.support_prior_joiners = args["support_prior_joiners"].as<bool>();
  options.segment_case = args["segment_case"].as<bool>();
  options.segment_numbers = args["segment_numbers"].as<bool>();
  options.segment_alphabet_change = args["segment_alphabet_change"].as<bool>();
  options.segment_alphabet = args["segment_alphabet"].as<std::vector<std::string>>();
  if (options.segment_alphabet.size() == 1 && options.segment_alphabet[0].empty())
    options.segment_alphabet.clear();
  return options;
}
