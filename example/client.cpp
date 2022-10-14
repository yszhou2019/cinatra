#include "../include/cinatra.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <chrono>

using namespace std::chrono;
using namespace cinatra;

void print(const response_data& result)
{
    print(result.ec, result.status, result.resp_body, result.resp_headers.second);
}

TEST(ClientTest, SyncTest)
{
    auto client = cinatra::client_factory::instance().new_client();
    auto routine = [&client](std::string url) {
        std::cout << "url " << url << std::endl;
        {
            auto start = steady_clock::now();
            response_data result = client->get(url);
            std::cout << "sync get " << result.ec << ", takes " << duration_cast<milliseconds>(steady_clock::now() - start).count() << " ms" << std::endl;
        }
        {
            auto start = steady_clock::now();
            std::cout << "sync post " << client->post(url, "hello", req_content_type::json).ec << ", takes " << duration_cast<milliseconds>(steady_clock::now() - start).count() << " ms" << std::endl;
        }
    };
    routine("http://www.baidu.com");
    routine("http://cn.bing.com");
}

TEST(ClientTest, AsyncTest)
{
    auto client = cinatra::client_factory::instance().new_client();
    auto routine = [&client](std::string url) {
        std::cout << "url " << url << std::endl;
        {
            auto start = steady_clock::now();
            client->async_get(url, [&url, &start](response_data data) { std::cout << "async get tasks " << duration_cast<milliseconds>(steady_clock::now() - start).count() << " ms" << std::endl; });
        }
        {
            auto start = steady_clock::now();
            client->async_post(url, "hello", [&url, &start](response_data data) { std::cout << "async post tasks " << duration_cast<milliseconds>(steady_clock::now() - start).count() << " ms" << std::endl; });
        }
    };
    routine("http://www.baidu.com");
    routine("http://cn.bing.com");
}

TEST(ClientTest, DownloadTest)
{
    std::string uri = "http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx";

    {
        auto client = cinatra::client_factory::instance().new_client();
        // Note: if the dest file has already exist, the file will be appened.
        // If you want to download a new file, make sure no such a file with the
        // same name. You could set the start position of the download file, eg:
        // int64_t start_pos = 100;
        // client->download(uri, "test1.jpg", start_pos, [](response_data data)...
        client->download(uri, "test1.jpg", [](response_data data) {
            if (data.ec)
            {
                std::cout << data.ec.message() << "\n";
                return;
            }

            std::cout << "finished download\n";
        });
    }

    {
        auto client = cinatra::client_factory::instance().new_client();
        client->download(uri, [](auto ec, auto data) {
            if (ec)
            {
                std::cout << ec.message() << "\n";
                return;
            }

            if (data.empty())
            {
                std::cout << "finished all \n";
            }
            else
            {
                std::cout << data.size() << "\n";
            }
        });
    }
}

TEST(ClientTest, UploadTest)
{
    std::string uri = "http://cn.bing.com/";
    auto client = cinatra::client_factory::instance().new_client();
    client->sync_upload(uri, "boost_1_72_0.7z", [](response_data data) {
        if (data.ec)
        {
            std::cout << data.ec.message() << "\n";
            return;
        }

        std::cout << data.ec << "\n"; // finished upload
    });
}

TEST(ClientTest, SMTPTest)
{
    boost::asio::io_context io_context;
    auto client = cinatra::smtp::get_smtp_client<cinatra::NonSSL>(io_context);
    smtp::email_server server{};
    server.server = "smtp.163.com";
    server.port = client.IS_SSL ? "465" : "25";
    server.user = "your_email@163.com";
    server.password = "your_email_password";

    smtp::email_data data{};
    data.filepath = ""; // some file as attachment.
    data.from_email = "your_email@163.com";
    data.to_email.push_back("to_some_email@163.com");
    // data.to_email.push_back("to_more_email@example.com");
    data.subject = "it is a test from cinatra smtp";
    data.text = "Hello cinatra smtp client";

    client.set_email_server(server);
    client.set_email_data(data);

    client.start();

    boost::system::error_code ec;
    io_context.run(ec);
}