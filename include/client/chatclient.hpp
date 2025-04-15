/**
 * @brief 聊天客户端
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef CHATCLIENT_H
#define CHATCLIENT_H
#include <string>
class ChatClient
{
public:
    ChatClient(const char *ip, const unsigned short port);
    ChatClient(const ChatClient &) = delete;
    ChatClient &operator=(const ChatClient &) = delete;
    ~ChatClient();
    void Start();

private:
    void onConnect();
    void onMessage();
    const char *ip_;
    unsigned short port_;
    int clientfd_;
    std::string recv_buffer_;
};
#endif
