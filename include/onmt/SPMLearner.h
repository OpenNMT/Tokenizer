#pragma once

#include <fstream>
#include <memory>
#include <unordered_map>

#include "onmt/opennmttokenizer_export.h"
#include "onmt/SubwordLearner.h"

namespace onmt
{

  class OPENNMTTOKENIZER_EXPORT SPMLearner: public SubwordLearner
  {
  public:
    SPMLearner(bool verbose,
               const std::string& opts,
               const std::string& input_filename,
               bool keep_vocab = false);
    SPMLearner(bool verbose,
               const std::vector<std::string>& opts,
               const std::string& input_filename,
               bool keep_vocab = false);
    SPMLearner(bool verbose,
               const std::unordered_map<std::string, std::string>& opts,
               const std::string& input_filename,
               bool keep_vocab);
    ~SPMLearner();

    void set_input_filename(const std::string& filename);

    void learn(std::ostream& os, const char* description = 0, bool verbose = false) override;
    void learn(const std::string& model_path,
               const char* description = 0,
               bool verbose = false) override;
  protected:
    void ingest_token(const std::string& token) override;
  private:
    std::string _args;
    std::string _input_filename;
    bool _keep_vocab;
    std::unique_ptr<std::ofstream> _input_stream;
  };

}
