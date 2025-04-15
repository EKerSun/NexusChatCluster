/**
 * @brief 基于muduo网络库的聊天服务的Server,提供基础的网络服务
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <unordered_map>
#include "rpcprovider.h"
#include "chatservice.hpp"

class ChatServer
{
public:
    void Start();

private:
    RpcProvider provider_;
    std::unordered_map<std::string, UserProto::UserServiceRpc::Service *> service_library_;
    std::unordered_set<std::string> keep_alive_method_set_;
};

#endif