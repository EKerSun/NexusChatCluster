#include "proxyserver.hpp"
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include "rpcheader.pb.h"
#include "rpcapplication.h"

void ProxyServer::Start()
{
    // 读取配置文件
    std::string ip = RpcApplication::GetInstance().GetConfig().Load("gateserverip");
    uint16_t port = atoi(RpcApplication::GetInstance().GetConfig().Load("gateserverport").c_str());
    muduo::net::InetAddress address(ip, port);
    // 创建TcpServer对象
    muduo::net::TcpServer server(&loop_, address, "ProxyServer");

    // 设置链接回调
    server.setConnectionCallback(std::bind(&ProxyServer::onConnection, this, std::placeholders::_1));
    // 设置消息回调
    server.setMessageCallback(std::bind(&ProxyServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // 设置线程数量
    server.setThreadNum(4);
    server.start();
    loop_.loop();
}

// 上报链接相关信息的回调函数
void ProxyServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        conn->shutdown();
    }
}
// 上报读写事件相关的回调函数
void ProxyServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp time)

{
    if (buffer->readableBytes() < 4)
    {
        return; // 数据不足，等待后续数据
    }
    const char *data = buffer->peek();
    uint32_t network_length = *(reinterpret_cast<const uint32_t *>(data));
    uint32_t length = ntohl(network_length); // 转换为本地字节序
    if (buffer->readableBytes() < 4 + length)
    {
        return; // 数据不足，等待后续数据
    }
    buffer->retrieve(4);
    std::string recv_buf = buffer->retrieveAllAsString();

    TheChat::RequestHeader request_header;
    if (!request_header.ParseFromString(recv_buf))
    {
        LOG_ERROR << "RPC header parse error!";
        return;
    }
    uint32_t message_id = request_header.message_id();
    MsgHandler msgHandler;
    if (!proxy_.GetMsgHandler(message_id, msgHandler))
    {
        LOG_ERROR << "RPC message_id = " << message_id << " is not register!";
        return;
    }
    msgHandler(request_header.content(), conn);
}
