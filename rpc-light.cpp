#include "server.hpp"
#include "client.hpp"
#include "result.hpp"

#include <string>
#include <iostream>
#include <chrono>

std::map<std::string, int> test_fn(int&& i)
{
    return {{"test", i}};
}

bool im_a_bool(const bool &b)
{
    return b;
}

auto print_notification(const std::string_view &msg)
{
    std::cout << msg << std::endl;
}

int main(int argc, char **argv)
{
    rpc_light::server_t server;
    rpc_light::client_t client;
    auto &dispatcher = server.get_dispatcher();

    dispatcher.add_method("bool", &im_a_bool);
    dispatcher.add_method("print_notification", &print_notification);
    dispatcher.add_method("test", &test_fn);
    auto batch = client.create_batch(
        client.create_request("print_notification", 1, "WE ARE PRINTING"),
        client.create_request("bool", 2, true),
        client.create_request("test", 3, 55));
    std::cout << batch << std::endl
              << std::endl;

    auto batch_server = server.handle_request(batch).get();
    auto batch_response = batch_server.get_response_str();
    std::cout << batch_response << std::endl
              << std::endl;

    auto batch_client = client.handle_response(batch_response).get();
}