#include "onmt/ITokenizer.h"

#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "onmt/SpaceTokenizer.h"

namespace onmt
{

  const std::string ITokenizer::feature_marker("￨");


  template <typename Output, typename Function>
  void work_loop(const Function& function,
                 std::queue<std::pair<std::promise<Output>, std::string>>& queue,
                 std::mutex& mutex,
                 std::condition_variable& cv,
                 const bool& end_requested)
  {
    while (true)
    {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait(lock, [&queue, &end_requested]{
        return !queue.empty() || end_requested;
      });

      if (end_requested)
      {
        lock.unlock();
        break;
      }

      auto work = std::move(queue.front());
      queue.pop();
      lock.unlock();

      auto& promise = work.first;
      auto& text = work.second;
      promise.set_value(function(text));
    }
  }

  static inline void log_progress(const size_t num_processed) {
    std::cerr << "... processed " << num_processed << " lines" << std::endl;
  }

  template <typename Output, typename Function, typename Writer>
  void process_stream(const Function& function,
                      const Writer& writer,
                      std::istream& in,
                      std::ostream& out,
                      size_t num_threads,
                      size_t buffer_size,
                      size_t report_every = 0)
  {
    std::string line;
    size_t num_processed = 0;
    if (num_threads <= 1) // Fast path for sequential processing.
    {
      while (std::getline(in, line))
      {
        writer(out, function(line));
        out << '\n';
        num_processed++;
        if (report_every > 0 && num_processed % report_every == 0)
          log_progress(num_processed);
      }
      out.flush();
      if (report_every > 0)
        log_progress(num_processed);
      return;
    }

    std::queue<std::pair<std::promise<Output>, std::string>> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool request_end = false;

    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
      workers.emplace_back(&work_loop<Output, Function>,
                           std::cref(function),
                           std::ref(queue),
                           std::ref(mutex),
                           std::ref(cv),
                           std::cref(request_end));

    std::queue<std::future<Output>> futures;

    auto pop_results = [&writer, &futures, &out, &num_processed, report_every](bool blocking) {
      static const auto zero_sec = std::chrono::seconds(0);
      while (!futures.empty()
             && (blocking
                 || futures.front().wait_for(zero_sec) == std::future_status::ready)) {
        writer(out, futures.front().get());
        out << '\n';
        futures.pop();
        num_processed++;
        if (report_every > 0 && num_processed % report_every == 0)
          log_progress(num_processed);
      }
    };

    while (std::getline(in, line))
    {
      {
        std::lock_guard<std::mutex> lock(mutex);
        queue.emplace(std::piecewise_construct,
                      std::forward_as_tuple(),
                      std::forward_as_tuple(std::move(line)));
        futures.emplace(queue.back().first.get_future());
      }

      cv.notify_one();
      if (futures.size() >= buffer_size)
        pop_results(/*blocking=*/false);
    }

    if (!futures.empty())
      pop_results(/*blocking=*/true);

    {
      std::lock_guard<std::mutex> lock(mutex);
      request_end = true;
    }

    cv.notify_all();
    for (auto& worker : workers)
      worker.join();
    out.flush();
    if (report_every > 0)
      log_progress(num_processed);
  }


  void ITokenizer::tokenize(const std::string& text,
                            std::vector<std::string>& words,
                            std::vector<std::vector<std::string> >& features,
                            std::unordered_map<std::string, size_t>&) const
  {
    tokenize(text, words, features);
  }

  void ITokenizer::tokenize(const std::string& text, std::vector<std::string>& words) const
  {
    std::vector<std::vector<std::string> > features;
    tokenize(text, words, features);
  }

  std::string ITokenizer::detokenize(const std::vector<std::string>& words) const
  {
    std::vector<std::vector<std::string> > features;
    return detokenize(words, features);
  }

  std::string ITokenizer::detokenize(const std::vector<std::string>& words,
                                     Ranges& ranges, bool merge_ranges) const
  {
    std::vector<std::vector<std::string>> features;
    return detokenize(words, features, ranges, merge_ranges);
  }

  std::string ITokenizer::detokenize(const std::vector<std::string>& words,
                                     const std::vector<std::vector<std::string> >& features,
                                     Ranges&, bool) const
  {
    return detokenize(words, features);
  }

  void ITokenizer::tokenize_stream(std::istream& in,
                                   std::ostream& out,
                                   size_t num_threads,
                                   bool verbose,
                                   size_t buffer_size) const
  {
    using Result = std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>>;
    auto function = [this](const std::string& text)
                    {
                      std::vector<std::string> words;
                      std::vector<std::vector<std::string>> features;
                      this->tokenize(text, words, features);
                      return Result(std::move(words), std::move(features));
                    };
    auto writer = [](std::ostream& os, const Result& result)
                  {
                    const auto& words = result.first;
                    const auto& features = result.second;
                    for (size_t i = 0; i < words.size(); ++i)
                    {
                      if (i > 0)
                        os << ' ';
                      os << words[i];

                      if (!features.empty())
                      {
                        for (size_t j = 0; j < features.size(); ++j)
                          os << feature_marker << features[j][i];
                      }
                    }
                  };
    if (verbose)
      std::cerr << "Start processing..." << std::endl;
    process_stream<Result>(function,
                           writer,
                           in,
                           out,
                           num_threads,
                           buffer_size,
                           verbose ? 100000 : 0);
  }

  void ITokenizer::detokenize_stream(std::istream& in, std::ostream& out) const
  {
    auto function = [this](const std::string& line) {
      std::vector<std::string> tokens;
      std::vector<std::vector<std::string>> features;
      SpaceTokenizer::get_instance().tokenize(line, tokens, features);
      return this->detokenize(tokens, features);
    };
    auto writer = [](std::ostream& os, const std::string& text) { os << text; };
    process_stream<std::string>(function, writer, in, out, /*num_threads=*/1, /*buffer_size=*/0);
  }

}
