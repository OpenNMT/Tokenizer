#pragma once

#include <fstream>
#include <memory>
#include <unordered_map>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordLearner.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT SentencePieceLearner: public SubwordLearner
  {
  public:
    SentencePieceLearner(bool verbose,
                         const std::string& opts,
                         const std::string& input_filename,
                         bool keep_vocab = false, 
                         bool keep_input_file = false);
    SentencePieceLearner(bool verbose,
                         const std::vector<std::string>& opts,
                         const std::string& input_filename,
                         bool keep_vocab = false, 
                         bool keep_input_file = false);
    SentencePieceLearner(bool verbose,
                         const std::unordered_map<std::string, std::string>& opts,
                         const std::string& input_filename,
                         bool keep_vocab = false, 
                         bool keep_input_file = false);
    ~SentencePieceLearner();

    void set_input_filename(const std::string& filename);

    void learn(std::ostream& os, const char* description = 0, bool verbose = false) override;
    void learn(const std::string& model_path,
               const char* description = 0,
               bool verbose = false) override;
  protected:
    void ingest_token_impl(const std::string& token) final;
  private:
    std::string _args;
    std::string _input_filename;
    bool _keep_vocab;
    std::unique_ptr<std::ofstream> _input_stream;
    bool _keep_input_file;
  };

}
