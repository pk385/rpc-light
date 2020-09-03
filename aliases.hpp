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
    using params_t = std::vector<value_t>;
    using method_t = std::function<value_t(params_t)>;
    using null_t = std::monostate;
    const char JSON_PROTO[] = "jsonrpc";
    const char JSON_VER[] = "2.0";
    const char JSON_ID[] = "id";
    const char JSON_RESULT[] = "result";
    const char JSON_METHOD[] = "method";
    const char JSON_PARAMS[] = "params";
    const char JSON_ERROR[] = "error";
    const char JSON_CODE[] = "code";
    const char JSON_MESSAGE[] = "message";
    const char JSON_DATA[] = "data";

} // namespace rpc_light