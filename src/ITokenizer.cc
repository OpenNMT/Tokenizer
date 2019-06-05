#include "onmt/ITokenizer.h"

#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "onmt/SpaceTokenizer.h"

namespace onmt
{

  const std::string ITokenizer::feature_marker("￨");


  template <typename Function>
  void work_loop(const Function& function,
                 std::queue<std::pair<std::promise<std::string>, std::string>>& queue,
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

  template <typename Function>
  void process_stream(const Function& function,
                      std::istream& in,
                      std::ostream& out,
                      size_t num_threads,
                      size_t buffer_size)
  {
    std::string line;
    if (num_threads <= 1) // Fast path for sequential processing.
    {
      while (std::getline(in, line))
        out << function(line) << std::endl;
      return;
    }

    std::queue<std::pair<std::promise<std::string>, std::string>> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool request_end = false;

    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
      workers.emplace_back(&work_loop<Function>,
                           std::cref(function),
                           std::ref(queue),
                           std::ref(mutex),
                           std::ref(cv),
                           std::cref(request_end));

    std::queue<std::future<std::string>> futures;

    auto pop_results = [&futures, &out](bool blocking) {
      static const auto zero_sec = std::chrono::seconds(0);
      while (!futures.empty()
             && (blocking
                 || futures.front().wait_for(zero_sec) == std::future_status::ready)) {
        out << futures.front().get() << std::endl;
        futures.pop();
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

  std::string ITokenizer::tokenize(const std::string& text) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    tokenize(text, words, features);

    return SpaceTokenizer::get_instance().detokenize(words, features);
  }

  std::string ITokenizer::detokenize(const std::string& text) const
  {
    std::vector<std::string> words;
    std::vector<std::vector<std::string> > features;

    SpaceTokenizer::get_instance().tokenize(text, words, features);

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
                                   size_t buffer_size) const
  {
    auto function = [this](const std::string& text) { return this->tokenize(text); };
    process_stream(function, in, out, num_threads, buffer_size);
  }

  void ITokenizer::detokenize_stream(std::istream& in, std::ostream& out) const
  {
    auto function = [this](const std::string& text) { return this->detokenize(text); };
    process_stream(function, in, out, /*num_threads=*/1, /*buffer_size=*/0);
  }

}
