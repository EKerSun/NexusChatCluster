/**
 * @brief 客户端服务
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef CLIENTSERVICE_H
#define CLIENTSERVICE_H
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <semaphore.h>
#include <atomic>
#include "user.pb.h"
#include "rpcheader.pb.h"

using ReadTaskHandler = std::function<void(const std::string &)>;
using SendTaskHandler = std::function<void(int)>;
class ClientService
{
public:
    ClientService();
    ~ClientService();

    void Init();

    static ClientService *Instance();
    void LoginMenu(int clientfd);
    void MainMenu(int clientfd);

    bool GetReadTaskHandler(int message_id, ReadTaskHandler &);

    void Register(int clientfd);
    void Login(int clientfd);
    void Quit(int clientfd);

    void AddFriend(int clientfd);
    void DeleteFriend(int clientfd);

    void CreateGroup(int clientfd);
    void DeleteGroup(int clientfd);
    void AddGroup(int clientfd);
    void QuitGroup(int clientfd);

    void FriendChat(int clientfd);
    void GroupChat(int clientfd);

    void RecvFriendMessage(const std::string &);
    void RecvGroupMessage(const std::string &);
    void RecvAddFriendMessage(const std::string &);
    void RecvDeleteFriendMessage(const std::string &);
    void RecvAddGroupMessage(const std::string &);
    void RecvQuitGroupMessage(const std::string &);
    void RecvDeleteGroupMessage(const std::string &);

    void Logout(int clientfd);

    void ShowCurrentUserData();
    void RegisterResponse(const std::string &);
    void LoginResponse(const std::string &);

    void AddFriendResponse(const std::string &);
    void DeleteFriendResponse(const std::string &);

    void CreateGroupResponse(const std::string &);
    void DeleteGroupResponse(const std::string &);
    void AddGroupResponse(const std::string &);
    void QuitGroupResponse(const std::string &);

    void FriendChatResponse(const std::string &);
    void GroupChatResponse(const std::string &);

    void LogoutResponse(const std::string &);

private:
    // 存储发送消息的任务
    std::unordered_map<int, SendTaskHandler> send_handler_map_;
    // 存储接收消息的任务
    std::unordered_map<int, ReadTaskHandler> read_handler_map_;
    // 记录当前系统登录的用户信息
    UserProto::User g_current_user_;
    // 记录当前用户的好友列表
    std::unordered_map<int, UserProto::FriendUser> g_friend_list_;
    // 记录当前用户的群组列表
    std::unordered_map<int, UserProto::Group> g_group_list_;
    // 记录当前用户的离线消息
    UserProto::OfflineMessageList g_offline_msg_list_;
    // 接收线程通知发送线程的信号量
    sem_t rwsem_;
    // 是否运行主界面
    std::atomic<bool> is_main_menu_loop_;
    // 是否初始化信号量
    bool sem_initialized_;
};

#endif
