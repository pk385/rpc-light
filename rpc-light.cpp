#include "server.hpp"
#include "client.hpp"
#include "result.hpp"

#include <string>
#include <iostream>
#include <chrono>

rpc_light::array_t test_fn(int i, int ii)
{
    return {{rpc_light::struct_t{{"test", i}, {"a", i ^ i}}, rpc_light::struct_t{{"n", i + 5}}}};
}

rpc_light::struct_t im_a_bool(bool b, int i)
{
    if (b)
        std::cout << "true" << std::endl;
    return {{"int", i}};
}

auto print_notification()
{
    std::cout << "notification" << std::endl;
}

struct s
{
    bool test()
    {
        return true;
    }
};

int main(int argc, char **argv)
{
    rpc_light::server_t server;
    rpc_light::client_t client;
    s mys;
    auto &dispatcher = server.get_dispatcher();

    dispatcher.add_param_mapping("bool", {{0, "zzz"}, {1, "myint"}});

    dispatcher.add_method("s::test", &s::test, mys);
    dispatcher.add_method("bool", &im_a_bool);
    dispatcher.add_method("test", &test_fn);
    dispatcher.add_method("print_notification", &print_notification);

    bool b = true;
    auto batch_request = client.create_batch(
        client.create_request("print_notification"),
        client.create_request("bool", "id1", {b, 777}),
        client.create_request("bool", 4, {{"zzz", true}, {"myint", 5}}),
        client.create_request("test", 3, {1, 2}));
    std::cout << "batch request: " << batch_request << std::endl
              << std::endl;

    auto batch_server = server.handle_request(batch_request).get();
    auto batch_response = batch_server.get_response_str();
    std::cout << "batch response: " << batch_response << std::endl
              << std::endl;

    auto batch_client = client.handle_response(batch_response).get();
    if (batch_client.has_error())
        std::cout << "error while parsing batch response" << std::endl;
}