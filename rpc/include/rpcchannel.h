/**
 * @author EkerSun
 * @date 2025.3.28
 * @brief 用于服务发现和远程调用
 */
#ifndef RPCCHHNEL_H
#define RPCCHHNEL_H

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

class TheRpcChannel : public google::protobuf::RpcChannel
{
public:
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);
};
#endif