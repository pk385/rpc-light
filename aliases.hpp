#pragma once

#include "value.hpp"

#include <string>
#include <functional>
#include <vector>
#include <map>

namespace rpc_light
{
    using array_t = std::vector<value_t>;
    using struct_t = std::map<std::string, value_t>;
    using method_t = std::function<value_t(array_t)>;
    using null_t = std::monostate;
    const char
        JSON_PROTO[] = "jsonrpc",
        JSON_VER[] = "2.0",
        JSON_ID[] = "id",
        JSON_RESULT[] = "result",
        JSON_METHOD[] = "method",
        JSON_PARAMS[] = "params",
        JSON_ERROR[] = "error",
        JSON_CODE[] = "code",
        JSON_MESSAGE[] = "message",
        JSON_DATA[] = "data";

} // namespace rpc_light