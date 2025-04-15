/**
 * @brief 处理聊天业务的具体方法，被ChatServer调用
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_set>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>

#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "groupmodel.hpp"
#include "usermodel.hpp"
#include "user.pb.h"
#include "redis.hpp"
#include "rpcheader.pb.h"

class ChatService
{
public:
    ChatService();
    void Init();
    static ChatService *Instance();
    std::unordered_map<std::string, UserProto::UserServiceRpc::Service *> GetServiceLibrary();
    std::unordered_set<std::string> GetKeepAliveMethodSet();

protected:
    // 数据操作类对象
    UserModel usermodel_;
    // 存储好友信息的操作类对象
    FriendModel friend_model_;
    // 存储群组
    GroupModel group_model_;
    // 存储离线消息
    OfflineMsgModel offline_msg_model_;

private:
    // 服务库
    std::unordered_map<std::string, UserProto::UserServiceRpc::Service *> service_library_;
    // 存储需要保持连接的方法
    std::unordered_set<std::string> keep_alive_method_set_;
};

/**
 * @brief 处理用户相关业务的类, 包括登录，注册，登出
 */
class UserService : public ChatService, public UserProto::UserServiceRpc
{
public:
    void Login(google::protobuf::RpcController *controller,
               const ::UserProto::LoginRequest *request,
               UserProto::LoginResponse *response,
               google::protobuf::Closure *done);
    void Register(google::protobuf::RpcController *controller,
                  const UserProto::RegisterRequest *request,
                  UserProto::RegisterResponse *response,
                  google::protobuf::Closure *done);
    void Logout(google::protobuf::RpcController *controller,
                const UserProto::LogoutRequest *request,
                UserProto::LogoutResponse *response,
                google::protobuf::Closure *done);
    static UserService *Instance();

private:
};

/**
 * @brief 处理好友相关业务的类，包括添加和删除好友
 */
class FriendService : public ChatService, public UserProto::FriendServiceRpc
{
public:
    void AddFriend(google::protobuf::RpcController *controller,
                   const ::UserProto::AddFriendRequest *request,
                   UserProto::AddFriendResponse *response,
                   google::protobuf::Closure *done);
    void DeleteFriend(google::protobuf::RpcController *controller,
                      const UserProto::DeleteFriendRequest *request,
                      UserProto::DeleteFriendResponse *response,
                      google::protobuf::Closure *done);
    static FriendService *Instance();

private:
};

/**
 * @brief 一对一聊天业务的类，包括好友聊天
 */
class OneChatService : public ChatService, public UserProto::OneChatServiceRpc
{
public:
    void FriendChat(google::protobuf::RpcController *controller,
                    const ::UserProto::FriendChatRequest *request,
                    UserProto::FriendChatResponse *response,
                    google::protobuf::Closure *done);
    static OneChatService *Instance();

private:
};

/**
 * @brief 群组业务，包括创建群组，加入群组，退出群组，删除群组
 */
class GroupService : public ChatService, public UserProto::GroupServiceRpc
{
public:
    void CreateGroup(google::protobuf::RpcController *controller,
                     const UserProto::CreateGroupRequest *request,
                     UserProto::CreateGroupResponse *response,
                     google::protobuf::Closure *done);
    void AddGroup(google::protobuf::RpcController *controller,
                  const ::UserProto::AddGroupRequest *request,
                  UserProto::AddGroupResponse *response,
                  google::protobuf::Closure *done);
    void QuitGroup(google::protobuf::RpcController *controller,
                   const ::UserProto::QuitGroupRequest *request,
                   UserProto::QuitGroupResponse *response,
                   google::protobuf::Closure *done);
    void DeleteGroup(google::protobuf::RpcController *controller,
                     const UserProto::DeleteGroupRequest *request,
                     UserProto::DeleteGroupResponse *response,
                     google::protobuf::Closure *done);

    static GroupService *Instance();

private:
};

/**
 * @brief 群组聊天业务，包括群组聊天
 */
class GroupChatService : public ChatService, public UserProto::GroupChatServiceRpc
{
public:
    void GroupChat(google::protobuf::RpcController *controller,
                   const UserProto::GroupChatRequest *request,
                   UserProto::GroupChatResponse *response,
                   google::protobuf::Closure *done);
    static GroupChatService *Instance();
};

#endif