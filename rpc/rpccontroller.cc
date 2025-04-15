#include "rpccontroller.h"

TheRpcController::TheRpcController()
{
    m_failed = false;
    m_errText = "";
}

void TheRpcController::Reset()
{
    m_failed = false;
    m_errText = "";
}

bool TheRpcController::Failed() const
{
    return m_failed;
}

std::string TheRpcController::ErrorText() const
{
    return m_errText;
}

void TheRpcController::SetFailed(const std::string &reason)
{
    m_failed = true;
    m_errText = reason;
}

void TheRpcController::SetConnection(const muduo::net::TcpConnectionPtr &conn)
{
    connection_ = conn;
}

muduo::net::TcpConnectionPtr TheRpcController::GetConnection() const
{
    return connection_.lock();
}

// 目前未实现具体的功能
void TheRpcController::StartCancel() {}
bool TheRpcController::IsCanceled() const { return false; }
void TheRpcController::NotifyOnCancel(google::protobuf::Closure *callback) {}