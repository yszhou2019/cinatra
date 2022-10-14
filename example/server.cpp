#include "../include/cinatra.hpp"
#include <iostream>

using namespace cinatra;

void print(const response_data &result) {
  print(result.ec, result.status, result.resp_body, result.resp_headers.second);
}

class qps {
public:
  void increase() { counter_.fetch_add(1, std::memory_order_release); }

  qps() : counter_(0) {
    thd_ = std::thread([this] {
      while (!stop_) {
        std::cout << "qps: " << counter_.load(std::memory_order_acquire)
                  << '\n';
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // counter_.store(0, std::memory_order_release);
      }
    });
  }

  ~qps() {
    stop_ = true;
    thd_.join();
  }

private:
  bool stop_ = false;
  std::thread thd_;
  std::atomic<uint32_t> counter_;
};

int main() {
  http_server server(std::thread::hardware_concurrency());
  bool r = server.listen("0.0.0.0", "8090");
  if (!r) {
    // LOG_INFO << "listen failed";
    return -1;
  }

  // server.on_connection([](auto conn) { return true; });
  server.set_http_handler<GET, POST>("/", [](request &, response &res) mutable {
    res.set_status_and_content(status_type::ok, "hello world");
    // res.set_status_and_content(status_type::ok, std::move(str));
  });

  server.set_http_handler<GET>("/plaintext", [](request &, response &res) {
    // res.set_status_and_content<status_type::ok,
    // res_content_type::string>("Hello, World!");
    res.set_status_and_content(status_type::ok, "Hello, World!",
                               req_content_type::string);
  });

  // chunked download
  // http://127.0.0.1:8080/assets/show.jpg
  // cinatra will send you the file, if the file is big file(more than 5M) the
  // file will be downloaded by chunked
  server.run();
  return 0;
}