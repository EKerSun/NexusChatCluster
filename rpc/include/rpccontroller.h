/**
 * @author EkerSun
 * @date 2025.3.28
 * @brief 自定义的控制器类，目前未设计具体方法
 */

#ifndef RPCCONTROLLER_H
#define RPCCONTROLLER_H

#include <google/protobuf/service.h>
#include <string>
#include "muduo/net/TcpConnection.h"

class TheRpcController : public google::protobuf::RpcController
{
public:
    TheRpcController();
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    void SetFailed(const std::string &reason);
    // ...其他必要方法实现...
    void SetConnection(const muduo::net::TcpConnectionPtr& conn);
    muduo::net::TcpConnectionPtr GetConnection() const;

    // 目前未实现具体的功能
    void StartCancel();
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure *callback);

private:
    bool m_failed;         // RPC方法执行过程中的状态
    std::string m_errText; // RPC方法执行过程中的错误信息
    std::weak_ptr<muduo::net::TcpConnection> connection_;
};
#endif