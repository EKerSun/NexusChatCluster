/**
 * @brief 代理服务器，接收来自客户端的请求，转发给其他服务器
 * @author EkerSun
 * @date 2025-4-15
 */

#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include <muduo/net/EventLoop.h>
#include "proxyservice.hpp"
class ProxyServer
{
public:
    void Start();

private:
    // 上报链接相关信息的回调函数
    void onConnection(const muduo::net::TcpConnectionPtr &conn);

    // 上报读写事件相关的回调函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buffer,
                   muduo::Timestamp time);
    ::muduo::net::EventLoop loop_;
    ProxyService proxy_;
};

#endif