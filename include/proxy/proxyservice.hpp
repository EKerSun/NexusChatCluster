/**
 * @brief 代理服务，通过解析的请求，调用zookeeper获取服务端地址，并调用远程服务
 * @author EkerSun
 * @date 2025-4-15
 */

#ifndef PROXYSERVICE_H
#define PROXYSERVICE_H
#include <functional>
#include <unordered_map>

#include "rpccontroller.h"
#include "rpcchannel.h"
#include "redis.hpp"
#include "user.pb.h"

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

class ConnectionManager
{
public:
    static ConnectionManager &instance();
    // 注册连接并生成唯一ID
    std::string registerConnection(const muduo::net::TcpConnectionPtr &conn, int user_id);
    // 根据ID获取连接
    muduo::net::TcpConnectionPtr getConnection(const std::string &connId);
    // 根据user_id获取连接ID
    std::string getConnId(int user_id);
    // 删除连接
    void removeConnection(int user_id);

private:
    std::mutex mutex_;
    boost::uuids::random_generator uuid_gen_;
    std::unordered_map<std::string, muduo::net::TcpConnectionPtr> connections_;
    std::unordered_map<int, std::string> connId_map_;
};

// 作为客户端代理，通过RPC调用远程服务
using MsgHandler = std::function<void(const std::string &, const muduo::net::TcpConnectionPtr &)>;

class ProxyService
{
public:
    ProxyService();
    ~ProxyService();

    void HandleRedisSubscribeMessage(std::string, std::string msg);
    bool GetMsgHandler(int msg_id, MsgHandler &message_handler);

    void Login(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void Register(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void Logout(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);

    void AddFriend(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void DeleteFriend(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);

    void CreateGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void DeleteGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void AddGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void QuitGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);

    void FriendChat(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);
    void GroupChat(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn);

private:
    // 用户业务代理
    UserProto::UserServiceRpc_Stub *user_stub_;
    // 好友业务代理
    UserProto::FriendServiceRpc_Stub *friend_stub_;
    // 好友聊天业务代理
    UserProto::OneChatServiceRpc_Stub *one_chat_stub_;
    // 群组业务代理
    UserProto::GroupServiceRpc_Stub *group_stub_;
    // 群组聊天业务
    UserProto::GroupChatServiceRpc_Stub *group_chat_stub_;
    // 存储所有的rpc服务调用方法
    std::unordered_map<int, MsgHandler> msg_handler_map_;
    // RPC控制器
    TheRpcController *rpc_controller_;
    // 存储所有在线用户的连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> user_connection_map_;
    // 本代理的id
    std::string local_id_;
    // 随机生成器
    boost::uuids::random_generator uuid_gen_;
    // 远程服务调用通道
    TheRpcChannel *rpc_channel_;
};

#endif