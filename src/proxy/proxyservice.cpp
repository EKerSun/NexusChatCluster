
#include "proxyservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <rpcheader.pb.h>

ConnectionManager &ConnectionManager::instance()
{
    static ConnectionManager mgr;
    return mgr;
}

// 注册连接并生成唯一ID
std::string ConnectionManager::registerConnection(const muduo::net::TcpConnectionPtr &conn, int user_id)
{
    std::string connId = boost::uuids::to_string(uuid_gen_());
    std::lock_guard<std::mutex> lock(mutex_);
    connections_[connId] = conn;
    connId_map_[user_id] = connId;
    return connId;
}

// 根据ID获取连接
muduo::net::TcpConnectionPtr ConnectionManager::getConnection(const std::string &connId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(connId);
    return (it != connections_.end()) ? it->second : nullptr;
}

// 根据user_id获取连接ID
std::string ConnectionManager::getConnId(int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connId_map_.find(user_id);
    return (it != connId_map_.end()) ? it->second : "";
}
// 删除连接
void ConnectionManager::removeConnection(int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connId_map_.find(user_id);
    connections_.erase(it->second);
    connId_map_.erase(user_id);
}

ProxyService::ProxyService()
{
    msg_handler_map_.insert(std::make_pair(REG_MSG, std::bind(&ProxyService::Register, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(LOGIN_MSG, std::bind(&ProxyService::Login, this, std::placeholders::_1, std::placeholders::_2)));

    msg_handler_map_.insert(std::make_pair(ADD_FRIEND_MSG, std::bind(&ProxyService::AddFriend, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(DELETE_FRIEND_MSG, std::bind(&ProxyService::DeleteFriend, this, std::placeholders::_1, std::placeholders::_2)));

    msg_handler_map_.insert(std::make_pair(CREATE_GROUP_MSG, std::bind(&ProxyService::CreateGroup, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(DELETE_GROUP_MSG, std::bind(&ProxyService::DeleteGroup, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(ADD_GROUP_MSG, std::bind(&ProxyService::AddGroup, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(QUIT_GROUP_MSG, std::bind(&ProxyService::QuitGroup, this, std::placeholders::_1, std::placeholders::_2)));

    msg_handler_map_.insert(std::make_pair(FRIEND_CHAT_MSG, std::bind(&ProxyService::FriendChat, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(GROUP_CHAT_MSG, std::bind(&ProxyService::GroupChat, this, std::placeholders::_1, std::placeholders::_2)));
    msg_handler_map_.insert(std::make_pair(LOGOUT_MSG, std::bind(&ProxyService::Logout, this, std::placeholders::_1, std::placeholders::_2)));

    rpc_channel_ = new TheRpcChannel();
    user_stub_ = new UserProto::UserServiceRpc_Stub(rpc_channel_);
    friend_stub_ = new UserProto::FriendServiceRpc_Stub(rpc_channel_);
    one_chat_stub_ = new UserProto::OneChatServiceRpc_Stub(rpc_channel_);
    group_stub_ = new UserProto::GroupServiceRpc_Stub(rpc_channel_);
    group_chat_stub_ = new UserProto::GroupChatServiceRpc_Stub(rpc_channel_);
    local_id_ = boost::uuids::to_string(uuid_gen_());
    if (Redis::Instance().connect())
    {
        Redis::Instance().init_notify_handler(std::bind(&ProxyService::HandleRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
        Redis::Instance().subscribe(local_id_);
    }
}
ProxyService::~ProxyService()
{
    delete rpc_channel_;
    delete user_stub_;
    delete friend_stub_;
    delete one_chat_stub_;
    delete group_stub_;
    delete group_chat_stub_;
    Redis::Instance().unsubscribe(local_id_);
}

void ProxyService::HandleRedisSubscribeMessage(std::string, std::string msg)
{
    UserProto::BroadcastMessage broadcast_message;
    if (!broadcast_message.ParseFromString(msg))
    {
        LOG_ERROR << "parse message error";
        return;
    }
    switch (broadcast_message.message_type())
    {
    case UserProto::MessageType::GROUP_CHAT:
    {

        UserProto::GroupMessage group_message;
        group_message.ParseFromString(broadcast_message.content());
        TheChat::ResponseHeader response_header;
        response_header.set_message_id(RECV_GROUP_MSG);
        response_header.set_content(group_message.content());
        std::string send_str = response_header.SerializeAsString();
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
        std::string total_send_data = header + send_str;

        for (auto &group_user : group_message.group_user_list().group_users())
        {
            // 检查是否在本节点
            std::string connId = ConnectionManager::instance().getConnId(group_user.user_id());
            if (!connId.empty())
            {
                muduo::net::TcpConnectionPtr conn = ConnectionManager::instance().getConnection(connId);
                conn->send(total_send_data);
            }
        }
        break;
    }
    case UserProto::MessageType::FRIEND_CHAT:
    {
        UserProto::FriendChatRequest request;
        request.ParseFromString(broadcast_message.content());
        int friend_id = request.receiver().user_id();
        TheChat::ResponseHeader response_header;
        response_header.set_message_id(RECV_FRIEND_MSG);
        response_header.set_content(broadcast_message.content());
        std::string send_str = response_header.SerializeAsString();
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
        std::string total_send_data = header + send_str;
        // 检查是否在本节点
        std::string connId = ConnectionManager::instance().getConnId(friend_id);
        if (!connId.empty())
        {
            muduo::net::TcpConnectionPtr conn = ConnectionManager::instance().getConnection(connId);
            conn->send(total_send_data);
        }
        break;
    }
    case UserProto::MessageType::ADD_FRIEND:
    {
        UserProto::AddFriendRequest request;
        request.ParseFromString(broadcast_message.content());
        int friend_id = request.frined_id();
        TheChat::ResponseHeader response_header;
        response_header.set_message_id(RECV_ADD_FRIEND_MSG);
        response_header.set_content(broadcast_message.content());
        std::string send_str = response_header.SerializeAsString();
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
        std::string total_send_data = header + send_str;
        // 检查是否在本节点
        std::string connId = ConnectionManager::instance().getConnId(friend_id);
        if (!connId.empty())
        {
            muduo::net::TcpConnectionPtr conn = ConnectionManager::instance().getConnection(connId);
            conn->send(total_send_data);
        }
        break;
    }
    case UserProto::MessageType::DELETE_FRIEND:
    {
        UserProto::DeleteFriendRequest request;
        request.ParseFromString(broadcast_message.content());
        int friend_id = request.frined_id();
        TheChat::ResponseHeader response_header;
        response_header.set_message_id(RECV_DELETE_FRIEND_MSG);
        response_header.set_content(broadcast_message.content());
        std::string send_str = response_header.SerializeAsString();
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;
        std::string connId = ConnectionManager::instance().getConnId(friend_id);
        if (!connId.empty())
        {
            muduo::net::TcpConnectionPtr conn = ConnectionManager::instance().getConnection(connId);
            conn->send(total_send_data);
        }
    }
    default:
        return;
    }
}

bool ProxyService::GetMsgHandler(int msg_id, MsgHandler &message_handler)
{
    auto it = msg_handler_map_.find(msg_id);
    if (it != msg_handler_map_.end())
    {
        message_handler = it->second;
        return true;
    }
    return false;
}

void ProxyService::Login(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::LoginRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse login request error";
        return;
    }
    UserProto::LoginResponse response;
    user_stub_->Login(rpc_controller_, &request, &response, nullptr);
    // 假如登录成功保存连接信息
    if (response.error_code() == 1)
    {
        // 将用户的连接信息存储到连接管理器
        std::string connId = ConnectionManager::instance().registerConnection(conn, request.user_id());
        // // 将用户的连接信息存储到 redis中
        Redis::Instance().hset(request.user_id(), local_id_);
    }
    // 发送响应
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize login response error";
    }
    response_header.set_message_id(LOGIN_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize login response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}

void ProxyService::Register(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::RegisterRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse register request error";
        return;
    }
    UserProto::RegisterResponse response;
    user_stub_->Register(rpc_controller_, &request, &response, nullptr);

    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize register response error";
    }
    response_header.set_message_id(REG_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize register response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}

void ProxyService::Logout(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::LogoutRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse logout request error";
        return;
    }
    UserProto::LogoutResponse response;
    user_stub_->Logout(rpc_controller_, &request, &response, nullptr);
    if (response.error_code())
    {
        // 将用户的连接信息从连接管理器中删除
        ConnectionManager::instance().removeConnection(request.user_id());
        // 将用户的连接信息从 redis中删除
        Redis::Instance().hdel(request.user_id());
    }
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize logout response error";
    }
    response_header.set_message_id(LOGOUT_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize logout response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}

void ProxyService::AddFriend(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::AddFriendRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse add friend request error";
        return;
    }
    UserProto::AddFriendResponse response;
    friend_stub_->AddFriend(rpc_controller_, &request, &response, nullptr);

    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize add friend response error";
    }
    response_header.set_message_id(ADD_FRIEND_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize add friend response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}

void ProxyService::DeleteFriend(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::DeleteFriendRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse delete friend request error";
        return;
    }
    UserProto::DeleteFriendResponse response;
    friend_stub_->DeleteFriend(rpc_controller_, &request, &response, nullptr);
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize delete friend response error";
    }
    response_header.set_message_id(DELETE_FRIEND_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize delete friend response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}
void ProxyService::FriendChat(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::FriendChatRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse friend chat request error";
        return;
    }
    UserProto::FriendChatResponse response;
    // 先由代理检查是否有连接，且在本节点
    std::string connId = ConnectionManager::instance().getConnId(request.receiver().user_id());
    if (!connId.empty())
    {
        muduo::net::TcpConnectionPtr friend_conn = ConnectionManager::instance().getConnection(connId);
        // 转发给好友
        TheChat::ResponseHeader client_response_friend;
        client_response_friend.set_message_id(RECV_FRIEND_MSG);
        client_response_friend.set_content(request_body);
        std::string send_str_friend = client_response_friend.SerializeAsString();
        int data_length_friend = send_str_friend.size();
        uint32_t network_length_friend = htonl(data_length_friend);
        std::string header_friend(reinterpret_cast<char *>(&network_length_friend), 4); // 4字节长度头
        std::string total_send_data_friend = header_friend + send_str_friend;
        friend_conn->send(total_send_data_friend);
        response.set_error_code(1);
    }
    else
    {
        // 用户不在当前节点，或者离线，由聊天服务发布到redis或者存储离线消息
        one_chat_stub_->FriendChat(rpc_controller_, &request, &response, nullptr);
    }
    // 反馈发送成功
    TheChat::ResponseHeader response_header;
    response_header.set_message_id(FRIEND_CHAT_MSG_ACK);
    response_header.set_content(response.SerializeAsString());
    std::string send_str = response_header.SerializeAsString();
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4); // 4字节长度头
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}
void ProxyService::CreateGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::CreateGroupRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse create group request error";
        return;
    }
    UserProto::CreateGroupResponse response;
    group_stub_->CreateGroup(rpc_controller_, &request, &response, nullptr);
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize create group response error";
    }
    response_header.set_message_id(CREATE_GROUP_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize create group response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4);
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}
void ProxyService::AddGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::AddGroupRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse add group request error";
        return;
    }
    UserProto::AddGroupResponse response;
    group_stub_->AddGroup(rpc_controller_, &request, &response, nullptr);
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize add group response error";
    }
    response_header.set_message_id(ADD_GROUP_MSG_ACK);
    response_header.set_content(response_body);
    std::string send_str;
    if (!response_header.SerializeToString(&send_str))
    {
        LOG_ERROR << "serialize add group response error";
    }
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4);
    std::string total_send_data = header + send_str;
    conn->send(total_send_data);
}
void ProxyService::QuitGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::QuitGroupRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse quit group request error";
        return;
    }
    UserProto::QuitGroupResponse response;
    group_stub_->QuitGroup(rpc_controller_, &request, &response, nullptr);
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize quit group response error";
    }
    response_header.set_message_id(QUIT_GROUP_MSG_ACK);
    response_header.set_content(response_body);
    if (!response_header.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize quit group response error";
    }
    int data_length = response_body.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4);
    std::string total_send_data = header + response_body;
    conn->send(total_send_data);
}
void ProxyService::DeleteGroup(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::DeleteGroupRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse delete group request error";
        return;
    }
    UserProto::DeleteGroupResponse response;
    group_stub_->DeleteGroup(rpc_controller_, &request, &response, nullptr);
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize delete group response error";
    }
    response_header.set_message_id(DELETE_GROUP_MSG_ACK);
    response_header.set_content(response_body);
    if (!response_header.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize delete group response error";
    }
    int data_length = response_body.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4);
    std::string total_send_data = header + response_body;
    conn->send(total_send_data);
}
void ProxyService::GroupChat(const std::string &request_body, const muduo::net::TcpConnectionPtr &conn)
{
    UserProto::GroupChatRequest request;
    if (!request.ParseFromString(request_body))
    {
        LOG_ERROR << "parse group chat request error";
        return;
    }
    UserProto::GroupChatResponse response;
    group_chat_stub_->GroupChat(rpc_controller_, &request, &response, nullptr);
    TheChat::ResponseHeader response_header;
    std::string response_body;
    if (!response.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize group chat response error";
    }
    response_header.set_message_id(GROUP_CHAT_MSG_ACK);
    response_header.set_content(response_body);
    if (!response_header.SerializeToString(&response_body))
    {
        LOG_ERROR << "serialize group chat response error";
    }
    int data_length = response_body.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4);
    std::string total_send_data = header + response_body;
    conn->send(total_send_data);
}
