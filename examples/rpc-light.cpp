#include "../include/rpc-light/server.hpp"
#include "../include/rpc-light/client.hpp"
#include <string>
#include <iostream>

rpc_light::struct_t return_struct(int i1, int i2)
{
    //return objects using initializer lists
    return {{"integer1", i1}, {"integer2", i2}};
}

rpc_light::array_t return_array(rpc_light::array_t array)
{
    //or use a normal object
    rpc_light::array_t double_array;
    for (auto &e : array)
        double_array.push_back(e.get_value<double>() * 2);
    return double_array;
}

void notification(std::string_view text)
{
    //use a notification if the client doesn't need a response
    std::cout << text << std::endl;
}

struct my_struct_t
{
    bool class_method()
    {
        //member functions work too
        return true;
    }
};

template <typename t>
t my_template(t arg)
{
    //so do templates
    return arg;
}

uint8_t implicit_convert(uint8_t smallint)
{
    //implicit conversions work too
    return smallint;
}

int explicit_convert(int integer)
{
    //explicit conversion using converter
    return integer;
}

bool error(int i)
{
    return true;
}

int main(int argc, char **argv)
{
    rpc_light::server_t server;
    rpc_light::client_t client;
    auto &dispatcher = server.get_dispatcher();

    //register a converter expression for complex or explicit conversion, like string -> int conversion
    rpc_light::global_converter.add_convert([](const std::string &str) { return std::stoi(str); });

    //register a function param mapping to accept params as json objects. usage: {param index, param name}
    dispatcher.add_param_mapping("return_struct", {{0, "myint1"}, {1, "myint2"}});

    dispatcher.add_method("return_struct", &return_struct);
    dispatcher.add_method("return_array", &return_array);
    dispatcher.add_method("notification", &notification);
    //use the 3 param overload to register member functions
    my_struct_t my_struct;
    dispatcher.add_method("my_struct.class_method", &my_struct_t::class_method, my_struct);
    dispatcher.add_method("my_template", &my_template<double>);
    dispatcher.add_method("imp_convert", &implicit_convert);
    dispatcher.add_method("exp_convert", &explicit_convert);
    dispatcher.add_method("error", &error);

    //create a batch to process multiple requests
    auto batch_request = client.create_batch(
        client.create_request("return_struct", 1, {{"myint1", 5}, {"myint2", 10}}),
        client.create_request("return_array", 2, {rpc_light::array_t{1.0, 2.0, 3.0}}),
        client.create_request("notification", {"some text."}),
        client.create_request("my_struct.class_method", 3),
        client.create_request("my_template", 4, {1.23456789}),
        client.create_request("imp_convert", 5, {2.5}),
        client.create_request("exp_convert", 6, {"100"}),
        client.create_request("error", 7, {"this will error."}));

    std::cout << "batch request: " << batch_request << std::endl
              << std::endl;

    //server request handler runs in a worker thread and returns a future
    auto batch_server = server.handle_request(batch_request).get();

    //get the server response serialization as string to be sent to the client
    auto batch_response = batch_server.get_response_str();

    if (batch_server.is_batch())
    {
        for (auto &e : batch_server.get_batch())
        {
            if (e.has_error())
                std::cout << "id: " << e.get_id().get_value<int>() << ": server error: " << e.get_message() << " " << e.get_data().get_value<std::string>() << std::endl;
        }
    }

    std::cout << "batch response: " << batch_response << std::endl
              << std::endl;

    //client response handler also uses worker thread
    auto batch_client = client.handle_response(batch_response).get();
    if (batch_client.is_batch())
    {
        for (auto &e : batch_client.get_batch())
        {
            if (e.has_error())
                std::cout << "id: " << e.get_id().get_value<int>() << ": client error: " << e.get_message() << " " << e.get_data().get_value<std::string>() << std::endl;
        }
    }
}