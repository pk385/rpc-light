# rpc-light

**rpc-light** is a JSON-RPC 2.0 client and server implementation that takes advantage of improvements and features added to the C++ standard library with the release of C++17.

There are multiple features and qualities of this library that might make it right for your next project:
* simple and lightweight
* fully compliant with [JSON-RPC 2.0 specification](https://www.jsonrpc.org/specification), including named parameters and batch processing
* easily bind to any function the accepts or returns JSON compatible types without modification
* use any types that are implicitly covertible to JSON types
* (TODO) ability to register converters for more complex conversions
* transport agnostic, bring your own transport
* header-only, easy to add to your project
  
## Requirements
* a C++17 capable compiler
* [RapidJSON](https://github.com/Tencent/rapidjson/)

## Installation
* copy the required header files to your project include path

## Usage
the code below demonstrates both a client and server example
```C++
#include "server.hpp"
#include "client.hpp"

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

uint8_t convert(uint8_t smallint)
{
    //implicit conversions work too
    return smallint;
}

bool error(int i)
{
    return true;
}

int main(int argc, char **argv)
{
    rpc_light::client_t client;
    rpc_light::server_t server;
    auto &dispatcher = server.get_dispatcher();

    //register a function param mapping to accept params as json objects. usage: {param index, param name}
    dispatcher.add_param_mapping("return_struct", {{0, "myint1"}, {1, "myint2"}});

    dispatcher.add_method("return_struct", &return_struct);
    dispatcher.add_method("return_array", &return_array);
    dispatcher.add_method("notification", &notification);
    //use the 3 param overload to register member functions
    my_struct_t my_struct;
    dispatcher.add_method("my_struct.class_method", &my_struct_t::class_method, my_struct);
    dispatcher.add_method("my_template", &my_template<double>);
    dispatcher.add_method("convert", &convert);
    dispatcher.add_method("error", &error);

    //create a batch to process multiple requests
    auto batch_request = client.create_batch(
        client.create_request("return_struct", 1, {5, 10}),
        client.create_request("return_array", 2, {rpc_light::array_t{1.0, 2.0, 3.0}}),
        client.create_request("notification", {"some text."}),
        client.create_request("my_struct.class_method", 3),
        client.create_request("my_template", 4, {1.23456789}),
        client.create_request("convert", 5, {2.5}),
        client.create_request("error", 6, {"this will error."}));

    std::cout << "batch request: " << batch_request << std::endl
              << std::endl;

    //server request handler runs in a new thread and returns a future
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

    //client response handler also uses std::async
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
