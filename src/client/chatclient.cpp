
#include "chatclient.hpp"
#include "clientservice.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <string.h>
#include <thread>

ChatClient::ChatClient(const char *ip, const unsigned short port)
    : ip_(ip), port_(port) {}
ChatClient::~ChatClient()
{
    if (clientfd_ != -1)
        close(clientfd_);
}
void ChatClient::Start()
{
    // 创建client端的socket
    clientfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd_)
    {
        std::cerr << "socket create error" << std::endl;
        exit(-1);
    }
    onConnect();
    ClientService::Instance()->Init();
    std::thread read_tesk(std::bind(&ChatClient::onMessage, this));
    read_tesk.detach();
    ClientService::Instance()->LoginMenu(clientfd_);
}
void ChatClient::onConnect()
{
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port_);
    server.sin_addr.s_addr = inet_addr(ip_);
    // client和server进行连接
    if (-1 == connect(clientfd_, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        std::cerr << "Connect server error!" << std::endl;
        close(clientfd_);
        exit(-1);
    }
}
// 接收线程
void ChatClient::onMessage()
{
    for (;;)
    {
        // 首先尝试接收新数据
        char temp_buffer[1024];
        int len_recv = recv(clientfd_, temp_buffer, sizeof(temp_buffer), 0);
        if (len_recv == 0)
        {
            close(clientfd_);
            std::cerr << "Server close!" << std::endl;
            exit(-1);
        }
        if (len_recv < 0)
        {
            close(clientfd_);
            std::cerr << "Recv data error!" << std::endl;
            exit(-1);
        }
        recv_buffer_.append(temp_buffer, len_recv); // 将数据追加到缓冲区
        if (recv_buffer_.size() < 4)
            continue; // 数据不足，等待下次接收

        // 读取长度头（不移动指针）
        uint32_t network_length;
        memcpy(&network_length, recv_buffer_.data(), sizeof(network_length));
        uint32_t data_length = ntohl(network_length);

        // 检查是否有足够的数据内容
        if (recv_buffer_.size() < 4 + data_length)
            continue;
        ; // 数据不足，等待下次接收
        std::string protobuf_data = recv_buffer_.substr(4, data_length);
        recv_buffer_.erase(0, 4 + data_length); // 移除已处理的数据

        TheChat::ResponseHeader response_header;
        if (!response_header.ParseFromString(protobuf_data))
        {
            std::cerr << "Parse client response error!" << std::endl;
            continue;
        }
        uint32_t message_id = response_header.message_id();
        ReadTaskHandler msg_handler;
        if (!ClientService::Instance()->GetReadTaskHandler(message_id, msg_handler))
        {
            std::cerr << "Get client response handler error!" << std::endl;
            continue;
        }
        msg_handler(response_header.content());
    }
}
