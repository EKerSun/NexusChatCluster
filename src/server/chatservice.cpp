#include "chatservice.hpp"
#include <functional>
#include <muduo/base/Logging.h>
#include <rpccontroller.h>
#include "public.hpp"

ChatService::ChatService()
{
}
void ChatService::Init()
{
    service_library_.insert(std::make_pair(UserService::Instance()->GetDescriptor()->name(), UserService::Instance()));
    service_library_.insert(std::make_pair(FriendService::Instance()->GetDescriptor()->name(), FriendService::Instance()));
    service_library_.insert(std::make_pair(OneChatService::Instance()->GetDescriptor()->name(), OneChatService::Instance()));
    service_library_.insert(std::make_pair(GroupService::Instance()->GetDescriptor()->name(), GroupService::Instance()));
    service_library_.insert(std::make_pair(GroupChatService::Instance()->GetDescriptor()->name(), GroupChatService::Instance()));
    if (!Redis::Instance().is_connected())
    {
        Redis::Instance().connect();
    }
}
ChatService *ChatService::Instance()
{
    static ChatService service;
    return &service;
}

// 获取服务库
std::unordered_map<std::string, UserProto::UserServiceRpc::Service *> ChatService::GetServiceLibrary()
{
    return service_library_;
}

std::unordered_set<std::string> ChatService::GetKeepAliveMethodSet()
{
    return keep_alive_method_set_;
}
UserService *UserService::Instance()
{
    static UserService service;
    return &service;
}
// 处理登录业务
void UserService::Login(google::protobuf::RpcController *controller,
                        const UserProto::LoginRequest *request,
                        UserProto::LoginResponse *response,
                        google::protobuf::Closure *done)
{
    LOG_INFO << "Do login business!";
    int user_id = request->user_id();
    std::string password = request->user_password();
    UserProto::User user;
    if (user_id <= 0)
    {
        // 用户ID不合法，错误码设置为2
        response->set_error_code(2);
    }
    else if (!usermodel_.Query(user_id, user))
    {
        // 用户ID不存在，错误码设置为3
        response->set_error_code(3);
    }
    else if (user.user_password() != password)
    {
        // 密码错误，错误码设置为4
        response->set_error_code(4);
    }
    else if (user.user_state() == "online")
    {
        // 用户已登录，错误码设置为5
        response->set_error_code(5);
    }
    else
    {
        // 登录成功, 错误码设置为1
        response->set_error_code(1);
        user.set_user_state("online");
        response->mutable_user()->CopyFrom(user);

        // 在数据库中，把用户状态设置为在线
        usermodel_.UpdateState(user);

        // 查询该用户的好友信息并返回
        friend_model_.Query(user_id, response->mutable_friend_list());

        // 查询该用户的群组列表并返回
        group_model_.QueryGroups(user_id, response->mutable_group_list());

        // 查询该用户的离线消息并返回
        offline_msg_model_.Query(user_id, response->mutable_offline_message_list());
        offline_msg_model_.QueryGroupMsg(response->mutable_group_list(), user.last_offline_time(), response->mutable_offline_message_list());
        offline_msg_model_.Remove(user_id);
    }
    done->Run();
}

void UserService::Register(google::protobuf::RpcController *controller,
                           const UserProto::RegisterRequest *request,
                           UserProto::RegisterResponse *response,
                           google::protobuf::Closure *done)
{
    LOG_INFO << "Do register business!";
    UserProto::User user;
    user.set_user_name(request->user_name());
    user.set_user_password(request->user_password());
    user.set_user_state("offline");
    if (user.user_name().empty() || user.user_password().empty())
    {
        // 用户名或密码为空，错误码设置为2
        response->set_error_code(2);
    }
    else if (!usermodel_.Insert(user))
    {
        // 插入用户失败，错误码设置为3
        response->set_error_code(3);
    }
    else
    {
        // 注册成功
        response->set_error_code(1);
        response->set_user_id(user.user_id());
    }
    done->Run();
}

void UserService::Logout(google::protobuf::RpcController *controller,
                         const UserProto::LogoutRequest *request,
                         UserProto::LogoutResponse *response,
                         google::protobuf::Closure *done)
{
    LOG_INFO << "Do logout business!";
    int user_id = request->user_id();

    UserProto::User user;
    user.set_user_id(user_id);
    user.set_user_state("offline");
    if (!usermodel_.UpdateState(user))
    {
        response->set_error_code(2);
    }
    else
    {
        response->set_error_code(1);
    }
    done->Run();
}

FriendService *FriendService::Instance()
{
    static FriendService service;
    return &service;
}

void FriendService::AddFriend(google::protobuf::RpcController *controller,
                              const ::UserProto::AddFriendRequest *request,
                              UserProto::AddFriendResponse *response,
                              google::protobuf::Closure *done)

{
    LOG_INFO << "Add friend business!";
    int user_id = request->user_id();
    int friend_id = request->frined_id();
    UserProto::User user;
    // 合法性检查
    if (user_id == friend_id || user_id <= 0 || friend_id <= 0)
    {
        // 用户ID不合法，错误码设置为2
        response->set_error_code(2);
    }
    else if (!usermodel_.UserIsExist(friend_id))
    {
        // 用户ID不存在，错误码设置为3
        response->set_error_code(3);
    }
    else if (friend_model_.IsFriend(user_id, friend_id))
    {
        // 已经是好友，错误码设置为4
        response->set_error_code(4);
    }
    else if (!usermodel_.Query(friend_id, user))
    {
        // 查询失败，错误码设置为5
        response->set_error_code(5);
    }
    else if (!friend_model_.Insert(user_id, friend_id))
    {
        // 存储失败，错误码设置为6
        response->set_error_code(6);
    }
    else
    {
        // 存储成功，错误码设置为1
        response->set_error_code(1);
        response->mutable_friend_user()->set_friend_id(friend_id);
        response->mutable_friend_user()->set_friend_name(user.user_name());
        response->mutable_friend_user()->set_friend_state(user.user_state());

        // 转发请求
        std::string proxy_info;
        Redis::Instance().hget(friend_id, proxy_info);
        if (!proxy_info.empty())
        {
            UserProto::BroadcastMessage message;
            message.set_message_type(UserProto::MessageType::ADD_FRIEND);
            message.set_content(request->SerializeAsString());
            Redis::Instance().publish(proxy_info, message.SerializeAsString());
        }
        else // 存储为离线消息
        {
            std::string request_info = "A add friend request from " + std::to_string(user_id);
            offline_msg_model_.Insert(friend_id, user_id, request->SerializeAsString());
        }
    }
    done->Run();
}

void FriendService::DeleteFriend(google::protobuf::RpcController *controller,
                                 const UserProto::DeleteFriendRequest *request,
                                 UserProto::DeleteFriendResponse *response,
                                 google::protobuf::Closure *done)
{
    LOG_INFO << "Delete friend business!";
    int user_id = request->user_id();
    int friend_id = request->frined_id();
    // 合法性检查
    if (user_id == friend_id || user_id <= 0 || friend_id <= 0)
    {
        // 用户ID不合法，错误码设置为2
        response->set_error_code(2);
    }
    else if (!usermodel_.UserIsExist(friend_id))
    {
        // 用户ID不存在，错误码设置为3
        response->set_error_code(3);
    }
    else if (!friend_model_.IsFriend(user_id, friend_id))
    {
        // 不是好友，错误码设置为4
        response->set_error_code(4);
    }
    else if (!friend_model_.Delete(user_id, friend_id))
    {
        // 删除失败
        response->set_error_code(5);
    }
    else
    { // 删除成功
        response->set_error_code(1);
        // 转发请求
        std::string proxy_info;
        Redis::Instance().hget(friend_id, proxy_info);
        if (!proxy_info.empty())
        {
            UserProto::BroadcastMessage message;
            message.set_message_type(UserProto::MessageType::DELETE_FRIEND);
            message.set_content(request->SerializeAsString());
            Redis::Instance().publish(proxy_info, message.SerializeAsString());
        }
        else // 存储为离线消息
        {
            std::string request_info = "A delete friend request from " + std::to_string(user_id);
            offline_msg_model_.Insert(friend_id, user_id, request->SerializeAsString());
        }
    }
    done->Run();
}

OneChatService *OneChatService::Instance()
{
    static OneChatService service;
    return &service;
}

void OneChatService::FriendChat(google::protobuf::RpcController *controller,
                                const ::UserProto::FriendChatRequest *request,
                                UserProto::FriendChatResponse *response,
                                google::protobuf::Closure *done)
{
    LOG_INFO << "Friend chat business!";
    int user_id = request->sender().user_id();
    int friend_id = request->receiver().user_id();
    std::string proxy_info;
    if (!Redis::Instance().hget(friend_id, proxy_info))
    {
        // 查询用户连接信息失败
        response->set_error_code(3);
    }
    else if (!proxy_info.empty())
    {
        // 用户在连接中
        UserProto::BroadcastMessage broadcast_message;
        broadcast_message.set_message_type(UserProto::MessageType::FRIEND_CHAT);
        broadcast_message.set_content(request->SerializeAsString());
        if (!Redis::Instance().publish(proxy_info, broadcast_message.SerializeAsString()))
        {
            // 发布失败
            response->set_error_code(4);
        }
        else
        {
            // 发布成功
            response->set_error_code(1);
        }
    }
    else
    {
        LOG_INFO << "OFFLINE";
        if (!offline_msg_model_.Insert(friend_id, user_id, request->content()))
        {
            // 插入离线消息失败
            response->set_error_code(5);
        }
        else
        {
            // 插入离线消息成功
            response->set_error_code(1);
        }
    }
    done->Run();
}

GroupService *GroupService::Instance()
{
    static GroupService service;
    return &service;
}

void GroupService::CreateGroup(google::protobuf::RpcController *controller,
                               const ::UserProto::CreateGroupRequest *request,
                               UserProto::CreateGroupResponse *response,
                               google::protobuf::Closure *done)
{
    LOG_INFO << "Create group business!";
    int user_id = request->user_id();
    std::string group_name = request->group_name();
    std::string group_desc = request->group_description();
    UserProto::Group group;
    group.set_group_name(group_name);
    group.set_group_description(group_desc);
    // 检查合法性
    if (group.group_name().empty())
    {
        response->set_error_code(2);
    }
    if (!group_model_.CreateGroup(group))
    {
        response->set_error_code(3);
    }
    else
    {
        // 创建群组成功, 将创建人加入群组中
        std::string role = "admin";
        if (!group_model_.AddGroup(user_id, group.group_id(), role))
        {
            response->set_error_code(4);
        }
        else
        {
            response->set_error_code(1);
            response->mutable_group()->CopyFrom(group);
        }
    }
    done->Run();
}

void GroupService::AddGroup(google::protobuf::RpcController *controller,
                            const ::UserProto::AddGroupRequest *request,
                            UserProto::AddGroupResponse *response,
                            google::protobuf::Closure *done)
{
    LOG_INFO << "Add group business!";
    int user_id = request->user_id();
    int group_id = request->group_id();
    std::string role = "member";
    // 检查输入合法性
    if (group_id <= 0 || user_id <= 0)
    {
        response->set_error_code(2);
    }
    else if (group_model_.IsInGroup(user_id, group_id))
    {
        response->set_error_code(3);
    }
    else if (!group_model_.AddGroup(user_id, group_id, role))
    {
        response->set_error_code(4);
    }
    else
    {
        response->set_error_code(1);
        group_model_.QueryGroup(group_id, response->mutable_group());
    }
    done->Run();
}

void GroupService::QuitGroup(google::protobuf::RpcController *controller,
                             const ::UserProto::QuitGroupRequest *request,
                             UserProto::QuitGroupResponse *response,
                             google::protobuf::Closure *done)
{
    LOG_INFO << "Quit group business!";
    int user_id = request->user_id();
    int group_id = request->group_id();
    // 检查输入合法性
    if (group_id <= 0 || user_id <= 0)
    {
        response->set_error_code(2);
    }
    else if (!group_model_.IsInGroup(user_id, group_id))
    {
        response->set_error_code(3);
    }
    else if (!group_model_.QuitGroup(user_id, group_id))
    {
        response->set_error_code(4);
    }
    else
    {
        response->set_error_code(1);
        response->set_group_id(group_id);
    }
    done->Run();
}

void GroupService::DeleteGroup(google::protobuf::RpcController *controller,
                               const ::UserProto::DeleteGroupRequest *request,
                               UserProto::DeleteGroupResponse *response,
                               google::protobuf::Closure *done)
{
    LOG_INFO << "Delete group business!";
    int user_id = request->user_id();
    int group_id = request->group_id();
    std::string role;
    if (group_id <= 0 || user_id <= 0)
    {
        response->set_error_code(2);
    }
    else if (!group_model_.QueryGroupRole(user_id, group_id, role))
    {
        response->set_error_code(3);
    }
    else if (role != "admin")
    {
        response->set_error_code(4);
    }
    else if (!group_model_.DeleteGroup(group_id))
    {
        response->set_error_code(5);
    }
    else
    {
        response->set_error_code(1);
    }
    done->Run();
}

GroupChatService *GroupChatService::Instance()
{
    static GroupChatService service;
    return &service;
}

void GroupChatService::GroupChat(google::protobuf::RpcController *controller,
                                 const UserProto::GroupChatRequest *request,
                                 UserProto::GroupChatResponse *response,
                                 google::protobuf::Closure *done)
{
    LOG_INFO << "Group chat business!";
    int user_id = request->sender().user_id();
    int group_id = request->receiver().group_id();
    std::string content = request->content();
    UserProto::GroupUserList group_user_list;
    UserProto::GroupMessage group_message;
    UserProto::BroadcastMessage broadcast_message;

    broadcast_message.set_message_type(UserProto::MessageType::GROUP_CHAT);
    std::unordered_map<std::string, UserProto::GroupUserList> proxy_map;
    std::string proxy_info;
    if (!group_model_.QueryGroupUsers(user_id, group_id, &group_user_list))
    {
        response->set_error_code(2);
    }
    else
    {
        // 存储离线消息, 不在线用户根据上次登录事件拉取
        offline_msg_model_.InsertGroupMsg(user_id, group_id, content);
        for (auto it = group_user_list.group_users().begin(); it != group_user_list.group_users().end(); ++it)
        {
            if (it->user_id() != user_id)
            {
                Redis::Instance().hget(it->user_id(), proxy_info);
                if (!proxy_info.empty())
                {
                    proxy_map[proxy_info].add_group_users()->set_user_id(it->user_id());
                }
            }
        }
        group_message.set_content(request->SerializeAsString());
        for (auto it = proxy_map.begin(); it != proxy_map.end(); ++it)
        {
            proxy_info = it->first;
            group_message.mutable_group_user_list()->CopyFrom(it->second);
            broadcast_message.set_content(group_message.SerializeAsString());
            Redis::Instance().publish(proxy_info, broadcast_message.SerializeAsString());
        }
        response->set_error_code(1);
    }
    done->Run();
}