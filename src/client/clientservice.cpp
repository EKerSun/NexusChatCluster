
#include "clientservice.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include "public.hpp"
#include <unistd.h>
void handle_serialize_error(std::string &&str)
{
    std::cerr << "Serialize " << str << "request error!" << std::endl;
    std::cerr << "Please try again or input q to quit." << std::endl;
}
void handle_send_error(std::string &&str)
{
    std::cerr << "Send " << str << "request error!" << std::endl;
    std::cerr << "Please try again or input q to quit." << std::endl;
}
void handle_parse_error(std::string &&str)
{
    std::cerr << "Parse " << str << "response error!" << std::endl;
}
void handle_service_error(std::string &&str)
{
    std::cerr << str << "error, please try again!" << std::endl;
}
std::string GetCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
ClientService::ClientService()
{
    // 请求业务处理
    send_handler_map_.insert({REG_MSG, std::bind(&ClientService::Register, this, std::placeholders::_1)});
    send_handler_map_.insert({LOGIN_MSG, std::bind(&ClientService::Login, this, std::placeholders::_1)});
    send_handler_map_.insert({QUIT_MSG, std::bind(&ClientService::Quit, this, std::placeholders::_1)});

    send_handler_map_.insert({ADD_FRIEND_MSG, std::bind(&ClientService::AddFriend, this, std::placeholders::_1)});
    send_handler_map_.insert({DELETE_FRIEND_MSG, std::bind(&ClientService::DeleteFriend, this, std::placeholders::_1)});

    send_handler_map_.insert({CREATE_GROUP_MSG, std::bind(&ClientService::CreateGroup, this, std::placeholders::_1)});
    send_handler_map_.insert({DELETE_GROUP_MSG, std::bind(&ClientService::DeleteGroup, this, std::placeholders::_1)});
    send_handler_map_.insert({ADD_GROUP_MSG, std::bind(&ClientService::AddGroup, this, std::placeholders::_1)});
    send_handler_map_.insert({QUIT_GROUP_MSG, std::bind(&ClientService::QuitGroup, this, std::placeholders::_1)});

    send_handler_map_.insert({FRIEND_CHAT_MSG, std::bind(&ClientService::FriendChat, this, std::placeholders::_1)});
    send_handler_map_.insert({GROUP_CHAT_MSG, std::bind(&ClientService::GroupChat, this, std::placeholders::_1)});

    send_handler_map_.insert({LOGOUT_MSG, std::bind(&ClientService::Logout, this, std::placeholders::_1)});

    // 响应业务处理
    read_handler_map_.insert({RECV_FRIEND_MSG, std::bind(&ClientService::RecvFriendMessage, this, std::placeholders::_1)});
    read_handler_map_.insert({RECV_GROUP_MSG, std::bind(&ClientService::RecvGroupMessage, this, std::placeholders::_1)});
    read_handler_map_.insert({RECV_ADD_FRIEND_MSG, std::bind(&ClientService::RecvAddFriendMessage, this, std::placeholders::_1)});
    read_handler_map_.insert({RECV_DELETE_FRIEND_MSG, std::bind(&ClientService::RecvDeleteFriendMessage, this, std::placeholders::_1)});
    read_handler_map_.insert({RECV_ADD_GROUP_MSG, std::bind(&ClientService::RecvAddGroupMessage, this, std::placeholders::_1)});
    read_handler_map_.insert({RECV_QUIT_GROUP_MSG, std::bind(&ClientService::RecvQuitGroupMessage, this, std::placeholders::_1)});
    read_handler_map_.insert({RECV_DELETE_GROUP_MSG, std::bind(&ClientService::RecvDeleteGroupMessage, this, std::placeholders::_1)});

    read_handler_map_.insert({REG_MSG_ACK, std::bind(&ClientService::RegisterResponse, this, std::placeholders::_1)});
    read_handler_map_.insert({LOGIN_MSG_ACK, std::bind(&ClientService::LoginResponse, this, std::placeholders::_1)});

    read_handler_map_.insert({ADD_FRIEND_MSG_ACK, std::bind(&ClientService::AddFriendResponse, this, std::placeholders::_1)});
    read_handler_map_.insert({DELETE_FRIEND_MSG_ACK, std::bind(&ClientService::DeleteFriendResponse, this, std::placeholders::_1)});

    read_handler_map_.insert({CREATE_GROUP_MSG_ACK, std::bind(&ClientService::CreateGroupResponse, this, std::placeholders::_1)});
    read_handler_map_.insert({DELETE_GROUP_MSG_ACK, std::bind(&ClientService::DeleteGroupResponse, this, std::placeholders::_1)});
    read_handler_map_.insert({ADD_GROUP_MSG_ACK, std::bind(&ClientService::AddGroupResponse, this, std::placeholders::_1)});
    read_handler_map_.insert({QUIT_GROUP_MSG_ACK, std::bind(&ClientService::QuitGroupResponse, this, std::placeholders::_1)});

    read_handler_map_.insert({FRIEND_CHAT_MSG_ACK, std::bind(&ClientService::FriendChatResponse, this, std::placeholders::_1)});
    read_handler_map_.insert({GROUP_CHAT_MSG_ACK, std::bind(&ClientService::GroupChatResponse, this, std::placeholders::_1)});

    read_handler_map_.insert({LOGOUT_MSG_ACK, std::bind(&ClientService::LogoutResponse, this, std::placeholders::_1)});
}

ClientService::~ClientService()
{
    if (sem_initialized_)
        sem_destroy(&rwsem_);
}

void ClientService::Init()
{
    sem_init(&rwsem_, 0, 0);
    sem_initialized_ = true;
}

ClientService *ClientService::Instance()
{
    static ClientService service;
    return &service;
}

void ClientService::LoginMenu(int clientfd)
{
    std::string choice;
    while (true)
    {
        std::cout << "***************** LoginPage *****************" << std::endl;
        std::cout << REG_MSG << ".Register." << std::endl;
        std::cout << LOGIN_MSG << ".Login." << std::endl;
        std::cout << QUIT_MSG << ".Quit." << std::endl;
        std::cout << "*********************************************" << std::endl;

        if (!(std::cin >> choice))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        // 验证是否为纯数字
        if (!std::all_of(choice.begin(), choice.end(), ::isdigit))
        {
            std::cerr << "Invalid input. Please enter a number." << std::endl;
            continue;
        }
        if (atoi(choice.c_str()) < 1 || atoi(choice.c_str()) > 3)
        {
            std::cerr << "Invalid input. Please enter a number between 1 and 3." << std::endl;
            continue;
        }
        auto it = send_handler_map_.find(atoi(choice.c_str()));
        it->second(clientfd);
    }
}

void ClientService::MainMenu(int clientfd)
{
    std::string choice;
    while (is_main_menu_loop_)
    {
        std::cout << "***************** MainPage *****************" << std::endl;
        std::cout << ADD_FRIEND_MSG << ". AddFriend" << std::endl;
        std::cout << DELETE_FRIEND_MSG << ". DeleteFriend" << std::endl;
        std::cout << CREATE_GROUP_MSG << ". CreateGroup" << std::endl;
        std::cout << DELETE_GROUP_MSG << ". DeleteGroup" << std::endl;
        std::cout << ADD_GROUP_MSG << ". AddGroup" << std::endl;
        std::cout << QUIT_GROUP_MSG << ". QuitGroup" << std::endl;
        std::cout << FRIEND_CHAT_MSG << ". FriendChat" << std::endl;
        std::cout << GROUP_CHAT_MSG << ". GroupChat" << std::endl;
        std::cout << LOGOUT_MSG << ". Logout" << std::endl;
        std::cout << "*********************************************" << std::endl;
        if (!(std::cin >> choice))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (!std::all_of(choice.begin(), choice.end(), ::isdigit))
        {
            std::cerr << "Invalid input. Please enter a number." << std::endl;
            continue;
        }
        if (atoi(choice.c_str()) < ADD_FRIEND_MSG || atoi(choice.c_str()) > LOGOUT_MSG)
        {
            std::cerr << "Wrong Number, please input again!" << std::endl;
            continue;
        }
        auto it = send_handler_map_.find(atoi(choice.c_str()));
        it->second(clientfd);
    }
}

bool ClientService::GetReadTaskHandler(int message_id, ReadTaskHandler &handler)
{
    auto it = read_handler_map_.find(message_id);
    if (it == read_handler_map_.end())
        return false;
    handler = it->second;
    return true;
}

void ClientService::Register(int clientfd)
{
    while (true)
    {
        std::string name;
        std::cout << "Please input the username: ";
        if (!(std::cin >> name))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (name == "q")
            break;
        if (name.empty())
        {
            std::cerr << "Username cannot be empty! Please try again." << std::endl;
            continue;
        }
        std::string pwd_first, pwd_second;
        std::cout << "Please input your password:";
        if (!(std::cin >> pwd_first))
        {
            std::cerr << "Password input error. Clearing buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cout << "Please confirm your password:";
        if (!(std::cin >> pwd_second))
        {
            std::cerr << "Password confirmation error. Clearing buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (pwd_first != pwd_second)
        {
            std::cerr << "The two passwords are inconsistent! " << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }
        // 组织请求
        UserProto::RegisterRequest request;
        request.set_user_name(name);
        request.set_user_password(pwd_first);
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("register");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(REG_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("register");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;

        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("register");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
        break;
    }
    // 注册成功，进入登录页面
    LoginMenu(clientfd);
}

void ClientService::RegisterResponse(const std::string &response_body)
{
    UserProto::RegisterResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("register");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("register");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Register success!" << std::endl;
    std::cout << "Your user id is: " << response.user_id() << std::endl;
    sem_post(&rwsem_);
}

void ClientService::Login(int clientfd)
{
    while (true)
    {
        std::string user_id;
        std::cout << "Please input your user id:";
        if (!(std::cin >> user_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (user_id == "q")
            break;
        if (user_id.empty())
        {
            std::cerr << "User id cannot be empty! Please try again." << std::endl;
            continue;
        }
        // 检查用户ID是否为数字
        if (!std::all_of(user_id.begin(), user_id.end(), ::isdigit))
        {
            std::cerr << "User ID must be numeric." << std::endl;
            continue;
        }

        std::string password;
        std::cout << "Please input your password: ";
        std::cin >> password;
        // 组织请求
        UserProto::LoginRequest request;
        request.set_user_id(atoi(user_id.c_str()));
        request.set_user_password(password);
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("login");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(LOGIN_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("login");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;

        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("login");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
        if (!is_main_menu_loop_)
        {
            continue;
        }
        break;
    }
    MainMenu(clientfd);
}
void ClientService::LoginResponse(const std::string &response_body)
{
    UserProto::LoginResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("login");
        is_main_menu_loop_ = false;
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("login");
        is_main_menu_loop_ = false;
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Login success!" << std::endl;
    // 存储当前用户的信息
    g_current_user_.set_user_id(response.user().user_id());
    g_current_user_.set_user_name(response.user().user_name());
    g_current_user_.set_user_state(response.user().user_state());
    // 存储当前用户的好友列表
    g_friend_list_.clear();
    for (const UserProto::FriendUser &friend_user : response.friend_list().friend_users())
    {
        g_friend_list_.insert(std::make_pair(friend_user.friend_id(), friend_user));
    }
    // 存储当前用户的群组列表
    g_group_list_.clear();
    for (const UserProto::Group &group : response.group_list().groups())
    {
        g_group_list_.insert(std::make_pair(group.group_id(), group));
    }
    // 存储当前用户的离线消息
    g_offline_msg_list_ = response.offline_message_list();
    is_main_menu_loop_ = true;
    ShowCurrentUserData();
    sem_post(&rwsem_);
}

void ClientService::Quit(int clientfd)
{
    close(clientfd);
    sem_initialized_ = false;
    sem_destroy(&rwsem_);
    exit(0);
}

void ClientService::AddFriend(int clientfd)
{
    while (true)
    {
        // 组织请求
        UserProto::AddFriendRequest request;
        std::string friend_id;
        std::cout << "Please input the friend id: ";
        if (!(std::cin >> friend_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (friend_id.empty())
        {
            std::cerr << "Friend id cannot be empty." << std::endl;
            continue;
        }
        if (friend_id == "q")
            break;
        // 验证是否为纯数字
        if (!std::all_of(friend_id.begin(), friend_id.end(), ::isdigit))
        {
            std::cerr << "Invalid friend ID. Please enter a number." << std::endl;
            continue;
        }

        if (g_friend_list_.find(atoi(friend_id.data())) != g_friend_list_.end())
        {
            std::cerr << "You have already added this friend!" << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }
        request.set_user_id(g_current_user_.user_id());
        request.set_frined_id(atoi(friend_id.data()));
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("addfriend");
            continue;
        }

        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(ADD_FRIEND_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("addfriend");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;

        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("addfriend");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
    }
}

void ClientService::AddFriendResponse(const std::string &response_body)
{
    UserProto::AddFriendResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("addfriend");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("addfriend");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Add friend success!" << std::endl;
    g_friend_list_.insert(std::make_pair(response.friend_user().friend_id(), response.friend_user()));
    sem_post(&rwsem_);
}

void ClientService::DeleteFriend(int clientfd)
{
    while (true)
    {
        // 组织请求
        UserProto::DeleteFriendRequest request;
        std::string friend_id;
        std::cout << "Please input the friend id: ";
        if (!(std::cin >> friend_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (friend_id.empty())
        {
            std::cerr << "Friend id cannot be empty." << std::endl;
            continue;
        }
        if (friend_id == "q")
            break;
        // 验证是否为纯数字
        if (!std::all_of(friend_id.begin(), friend_id.end(), ::isdigit))
        {
            std::cerr << "Invalid friend ID. Please enter a number." << std::endl;
            continue;
        }

        if (g_friend_list_.find(atoi(friend_id.data())) == g_friend_list_.end())
        {
            std::cerr << "You have not added this friend!" << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }
        request.set_user_id(g_current_user_.user_id());
        request.set_frined_id(atoi(friend_id.data()));
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("delete friend");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(DELETE_FRIEND_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("delete friend");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;

        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("delete friend");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
    }
}

void ClientService::DeleteFriendResponse(const std::string &response_body)
{
    UserProto::DeleteFriendResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("delete friend");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("delete friend");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Delete friend success!" << std::endl;
    g_friend_list_.erase(response.frined_id());
    sem_post(&rwsem_);
}

void ClientService::CreateGroup(int clientfd)
{
    while (true)
    {
        std::cout << "Please input the group name:";
        std::string group_name;
        if (!(std::cin >> group_name))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (group_name == "q")
            break;

        std::cout << "Please input the group description:";
        std::string group_description;
        if (!(std::cin >> group_description))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (group_name == "q")
            break;

        // 组织请求
        UserProto::CreateGroupRequest request;
        request.set_user_id(g_current_user_.user_id());
        request.set_group_name(group_name);
        request.set_group_description(group_description);
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("create group");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(CREATE_GROUP_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("create group");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;
        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("create group");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
    }
}
void ClientService::CreateGroupResponse(const std::string &response_body)
{
    UserProto::CreateGroupResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("create group");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("create group");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Create group success!" << std::endl;
    std::cout << "Group id:" << response.group().group_id() << std::endl;
    g_group_list_.insert(std::make_pair(response.group().group_id(), response.group()));
    sem_post(&rwsem_);
}

void ClientService::DeleteGroup(int clientfd)
{
    while (true)
    {
        std::cout << "Please input the group id:";
        std::string group_id;
        if (!(std::cin >> group_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (group_id == "q")
            break;
        if (!std::all_of(group_id.begin(), group_id.end(), ::isdigit))
        {
            std::cerr << "Invalid group id. Please enter a number." << std::endl;
            continue;
        }
        if (g_group_list_.find(atoi(group_id.c_str())) == g_group_list_.end())
        {
            std::cerr << "You don't belong to this group." << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }

        // 组织请求
        UserProto::DeleteGroupRequest request;
        request.set_user_id(g_current_user_.user_id());
        request.set_group_id(atoi(group_id.c_str()));
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("delete group");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(DELETE_GROUP_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("delete group");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;
        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("delete group");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
    }
}
void ClientService::DeleteGroupResponse(const std::string &response_body)
{
    UserProto::DeleteGroupResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("delete group");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("delete group");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Delete group success!" << std::endl;
    g_group_list_.erase(response.group_id());
    sem_post(&rwsem_);
}

void ClientService::AddGroup(int clientfd)
{
    while (true)
    {
        std::cout << "Please input the group id:";
        std::string group_id;
        if (!(std::cin >> group_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (group_id == "q")
            break;
        if (!std::all_of(group_id.begin(), group_id.end(), ::isdigit))
        {
            std::cerr << "Invalid group id. Please enter a number." << std::endl;
            continue;
        }
        if (g_group_list_.find(atoi(group_id.data())) != g_group_list_.end())
        {
            std::cerr << "You have already added this group!" << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }

        // 组织请求
        UserProto::AddGroupRequest request;
        request.set_user_id(g_current_user_.user_id());
        request.set_group_id(atoi(group_id.data()));
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("add group");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(ADD_GROUP_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("add group");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;
        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("add group");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
    }
}

void ClientService::AddGroupResponse(const std::string &response_body)
{
    UserProto::AddGroupResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("add group");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("add group");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Add group success!" << std::endl;
    g_group_list_.insert(std::make_pair(response.group().group_id(), response.group()));
    sem_post(&rwsem_);
}

void ClientService::QuitGroup(int clientfd)
{
    while (true)
    {
        std::cout << "Please input the group id:";
        std::string group_id;
        if (!(std::cin >> group_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (group_id == "q")
            break;
        if (!std::all_of(group_id.begin(), group_id.end(), ::isdigit))
        {
            std::cerr << "Invalid group id. Please enter a number." << std::endl;
            continue;
        }
        if (g_group_list_.find(atoi(group_id.c_str())) == g_group_list_.end())
        {
            std::cerr << "You don't belong to this group." << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }
        // 组织请求
        UserProto::QuitGroupRequest request;
        request.set_user_id(g_current_user_.user_id());
        request.set_group_id(atoi(group_id.c_str()));
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            handle_serialize_error("quit group");
            continue;
        }
        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(QUIT_GROUP_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            handle_serialize_error("quit group");
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;
        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1)
        {
            handle_send_error("quit group");
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
    }
}

void ClientService::QuitGroupResponse(const std::string &response_body)
{
    UserProto::QuitGroupResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("quit group");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("quit group");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Quit group success!" << std::endl;
    g_group_list_.erase(response.group_id());
    sem_post(&rwsem_);
}

void ClientService::FriendChat(int clientfd)
{
    TheChat::RequestHeader request_header;
    UserProto::FriendChatRequest request;
    std::string content, request_str, send_str;
    request_header.set_message_id(FRIEND_CHAT_MSG);
    while (true)
    {
        std::cout << "Please input the friend id:";
        std::string friend_id;
        if (!(std::cin >> friend_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cin.get();
        if (friend_id == "q")
            break;
        if (!std::all_of(friend_id.begin(), friend_id.end(), ::isdigit))
        {
            std::cerr << "Invalid friend id. Please enter a number." << std::endl;
            continue;
        }
        if (g_friend_list_.find(atoi(friend_id.data())) == g_friend_list_.end())
        {
            std::cerr << "You have not added this friend!" << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }
        request.mutable_sender()->set_user_id(g_current_user_.user_id());
        request.mutable_sender()->set_user_name(g_current_user_.user_name());
        request.mutable_receiver()->set_user_id(atoi(friend_id.data()));

        while (true)
        {
            std::cout << "Please input the message:";
            if (!std::getline(std::cin, content))
            {
                std::cerr << "Input error!" << std::endl;
                std::cin.clear();                                                   // 重置输入流错误状态
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
                continue;
            }
            if (content == "q")
                break;
            // 组织请求
            request.set_time(GetCurrentTime());
            request.set_content(content);

            if (!request.SerializeToString(&request_str))
            {
                handle_serialize_error("friend chat");
                continue;
            }
            // 封装成客户端请求消息
            request_header.set_content(request_str);
            if (!request_header.SerializeToString(&send_str))
            {
                handle_serialize_error("friend chat");
                continue;
            }
            // 在发送前添加长度头
            int data_length = send_str.size();
            uint32_t network_length = htonl(data_length);
            std::string header(reinterpret_cast<char *>(&network_length), 4);
            std::string total_send_data = header + send_str;

            // 使用socket发送给网关
            int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
            if (send_len == -1)
            {
                handle_send_error("friend chat");
                continue;
            }
            sem_wait(&rwsem_);
        }
    }
}

void ClientService::FriendChatResponse(const std::string &response_body)
{
    UserProto::FriendChatResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("friend chat");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("friend chat");
        sem_post(&rwsem_);
        return;
    }
    sem_post(&rwsem_);
}

void ClientService::GroupChat(int clientfd)
{
    TheChat::RequestHeader request_header;
    UserProto::GroupChatRequest request;
    std::string content, request_str, send_str;
    request_header.set_message_id(GROUP_CHAT_MSG);
    while (true)
    {
        std::cout << "Please input the group id:";
        std::string group_id;
        if (!(std::cin >> group_id))
        {
            std::cerr << "Invalid input. Clearing input buffer..." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cin.get();
        if (group_id == "q")
            break;
        if (!std::all_of(group_id.begin(), group_id.end(), ::isdigit))
        {
            std::cerr << "Invalid group id. Please enter a number." << std::endl;
            continue;
        }
        if (g_group_list_.find(atoi(group_id.c_str())) == g_group_list_.end())
        {
            std::cerr << "You don't belong to this group." << std::endl;
            std::cerr << " Please try again or input q to quit." << std::endl;
            continue;
        }
        // 组织请求
        request.mutable_sender()->set_user_id(g_current_user_.user_id());
        request.mutable_sender()->set_user_name(g_current_user_.user_name());
        request.mutable_receiver()->set_group_id(atoi(group_id.c_str()));

        while (true)
        {
            std::cout << "Please input the message:";
            if (!std::getline(std::cin, content))
            {
                std::cerr << "Input error!" << std::endl;
                std::cin.clear();                                                   // 重置输入流错误状态
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 清空输入缓冲区
                continue;
            }
            if (content == "q")
                break;
            // 组织消息
            request.set_time(GetCurrentTime());
            request.set_content(content);

            if (!request.SerializeToString(&request_str))
            {
                handle_serialize_error("group chat");
                continue;
            }
            // 封装成客户端请求消息
            request_header.set_content(request_str);
            if (!request_header.SerializeToString(&send_str))
            {
                handle_serialize_error("group chat");
                continue;
            }
            // 在发送前添加长度头
            int data_length = send_str.size();
            uint32_t network_length = htonl(data_length);
            std::string header(reinterpret_cast<char *>(&network_length), 4);
            std::string total_send_data = header + send_str;
            // 使用socket发送给网关
            int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
            if (send_len == -1)
            {
                handle_send_error("group chat");
                continue;
            }
            sem_wait(&rwsem_);
        }
    }
}

void ClientService::GroupChatResponse(const std::string &response_body)
{
    UserProto::GroupChatResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("group chat");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("group chat");
        sem_post(&rwsem_);
        return;
    }
    sem_post(&rwsem_);
}

void ClientService::RecvFriendMessage(const std::string &response_body)
{
    UserProto::FriendChatRequest message;
    message.ParseFromString(response_body);
    std::cout << message.time() << " "
              << message.sender().user_id() << " "
              << message.sender().user_name() << " :"
              << message.content() << std::endl;
}

void ClientService::RecvGroupMessage(const std::string &response_body)
{
    std::cout << "rEcvGroupMessage" << std::endl;
    UserProto::GroupChatRequest message;
    message.ParseFromString(response_body);
    std::cout << message.time() << "  "
              << message.receiver().group_id() << " "
              << g_group_list_[message.receiver().group_id()].group_name() << " "
              << message.sender().user_name() << " :"
              << message.content() << std::endl;
}

void ClientService::RecvAddFriendMessage(const std::string &response_body)
{
    UserProto::AddFriendRequest request;
    request.ParseFromString(response_body);
    std::cout << "You have a new friend request from " << request.user_id() << std::endl;
    g_friend_list_[request.user_id()].set_friend_id(request.user_id());
}

void ClientService::RecvDeleteFriendMessage(const std::string &response_body)
{
    UserProto::DeleteFriendRequest request;
    request.ParseFromString(response_body);
    std::cout << "You have a new delete friend request from " << request.user_id() << std::endl;
    g_friend_list_.erase(request.user_id());
}

void ClientService::RecvAddGroupMessage(const std::string &response_body)
{
    UserProto::AddGroupRequest request;
    request.ParseFromString(response_body);
    std::cout << "User " << request.user_id() << " want to add group " << request.group_id() << std::endl;
    g_group_list_[request.group_id()].mutable_group_user_list()->add_group_users()->set_user_id(request.user_id());
}

void ClientService::RecvQuitGroupMessage(const std::string &response_body)
{
    UserProto::QuitGroupRequest request;
    request.ParseFromString(response_body);
    std::cout << "User " << request.user_id() << " want to quit group " << request.group_id() << std::endl;
    UserProto::GroupUserList *group_user_list = g_group_list_[request.group_id()].mutable_group_user_list();
    for (auto it = group_user_list->group_users().begin(); it != group_user_list->group_users().end(); it++)
    {
        if (it->user_id() == request.user_id())
        {
            group_user_list->mutable_group_users()->erase(it);
            return;
        }
    }
}
void ClientService::RecvDeleteGroupMessage(const std::string &response_body)
{
    UserProto::DeleteGroupRequest request;
    request.ParseFromString(response_body);
    std::cout << "User " << request.user_id() << " want to delete group " << request.group_id() << std::endl;
    g_group_list_.erase(request.group_id());
}

void ClientService::Logout(int clientfd)
{
    // 尝试10次
    int try_times = 1;
    UserProto::LogoutRequest request;
    request.set_user_id(g_current_user_.user_id());
    TheChat::RequestHeader request_header;
    request_header.set_message_id(LOGOUT_MSG);

    while (try_times <= 10)
    {
        // 组织请求
        std::string request_str;
        if (!request.SerializeToString(&request_str))
        {
            std::cerr << "Serialize logout request error!" << std::endl;
            std::cerr << "Now try the " << ++try_times << " times..." << std::endl;
            continue;
        }

        // 封装成客户端请求消息
        TheChat::RequestHeader request_header;
        request_header.set_message_id(LOGOUT_MSG);
        request_header.set_content(request_str);
        std::string send_str;
        if (!request_header.SerializeToString(&send_str))
        {
            std::cerr << "Serialize logout request error!" << std::endl;
            std::cerr << "Now try the " << ++try_times << " times..." << std::endl;
            continue;
        }
        // 在发送前添加长度头
        int data_length = send_str.size();
        uint32_t network_length = htonl(data_length);
        std::string header(reinterpret_cast<char *>(&network_length), 4);
        std::string total_send_data = header + send_str;

        // 使用socket发送给网关
        int send_len = send(clientfd, total_send_data.data(), total_send_data.size(), 0);
        if (send_len == -1 || send_len < total_send_data.size())
        {
            std::cerr << "Send logout message error!" << std::endl;
            std::cerr << "Now try the " << ++try_times << " times..." << std::endl;
            continue;
        }
        // 等待信号量，子线程处理完注册消息会通知
        sem_wait(&rwsem_);
        if (is_main_menu_loop_)
        {
            std::cerr << "Now try the " << ++try_times << " times..." << std::endl;
            continue;
        }
        break;
    }
}

void ClientService::LogoutResponse(const std::string &response_body)
{
    UserProto::LogoutResponse response;
    if (!response.ParseFromString(response_body))
    {
        handle_parse_error("logout");
        sem_post(&rwsem_);
        return;
    }
    if (response.error_code() != 1)
    {
        handle_service_error("logout");
        sem_post(&rwsem_);
        return;
    }
    std::cout << "Logout success!" << std::endl;
    g_current_user_.Clear();
    g_friend_list_.clear();
    g_group_list_.clear();
    g_offline_msg_list_.Clear();
    is_main_menu_loop_ = false;
    sem_post(&rwsem_);
}

void ClientService::ShowCurrentUserData()
{
    std::cout << "====================== Login Infomation ======================" << std::endl;
    std::cout << "  Current login user id: " << g_current_user_.user_id() << std::endl;
    std::cout << "  Current login user name: " << g_current_user_.user_name() << std::endl;
    std::cout << "----------------------Friend List---------------------" << std::endl;
    if (g_friend_list_.size() == 0)
    {
        std::cout << "  No friend." << std::endl;
    }
    else
    {
        for (auto it : g_friend_list_)
        {
            std::cout << "  ID: " << it.first << "  Name: " << it.second.friend_name() << std::endl;
        }
    }
    std::cout << "----------------------Group List----------------------" << std::endl;
    if (g_group_list_.size() == 0)
    {
        std::cout << "  No group." << std::endl;
    }
    else
    {
        for (auto it : g_group_list_)
        {
            std::cout << "  ID: " << it.first << "  Name: " << it.second.group_name() << std::endl;
        }
    }
    std::cout << "----------------------Offline Message----------------------" << std::endl;
    if (g_offline_msg_list_.ByteSizeLong() == 0)
        std::cout << "  No offline message." << std::endl;
    else
    {
        for (auto it : g_offline_msg_list_.messages())
        {
            if (it.message_type() == UserProto::MessageType::FRIEND_CHAT)
            {
                std::cout << it.time() << " "
                          << it.sender().user_id() << " "
                          << it.sender().user_name() << " :"
                          << it.content() << std::endl;
            }
        }
        for (auto it : g_offline_msg_list_.messages())
        {
            if (it.message_type() == UserProto::MessageType::GROUP_CHAT)
            {
                std::cout << it.time() << " "
                          << it.receiver().group_id() << " "
                          << g_group_list_[it.receiver().group_id()].group_name() << " "
                          << it.sender().user_name() << " :"
                          << it.content() << std::endl;
            }
        }
        g_offline_msg_list_.Clear();
    }
}