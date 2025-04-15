#include "rpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "rpcapplication.h"
#include "rpccontroller.h"
#include "zookeeperutil.h"

/**
 * @brief rpc的rpcchannel类
 * @param method 要远程调用的方法
 * @param controller rpc控制对象
 * @param request 请求参数，由客户端传入
 * @param response 响应参数，由远程服务端传入
 */
void TheRpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                               google::protobuf::RpcController *controller,
                               const google::protobuf::Message *request,
                               google::protobuf::Message *response,
                               google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    std::string request_str;
    if (!request->SerializeToString(&request_str))
    {
        controller->SetFailed("serialize request error!");
        return;
    }
    TheChat::RpcHeader rpc_header;
    rpc_header.set_service_name(service_name);
    rpc_header.set_method_name(method_name);
    rpc_header.set_params(request_str);

    std::string send_str;
    if (!rpc_header.SerializeToString(&send_str))
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 在发送前添加长度头
    int data_length = send_str.size();
    uint32_t network_length = htonl(data_length);
    std::string header(reinterpret_cast<char *>(&network_length), 4);
    std::string total_send_data = header + send_str;

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    // 服务发现
    ZooKeeperClient zookeeper_client;
    zookeeper_client.Start();
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string node_info;
    if (!zookeeper_client.GetData(method_path, node_info))
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }

    TheChat::ServiceMeta meta;
    if (!meta.ParseFromString(node_info))
    {
        controller->SetFailed("parse node info error!");
        return;
    }
    std::string ip = meta.ip();
    uint16_t port = meta.port();
    // 连接远程服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    if (-1 == send(clientfd, total_send_data.c_str(), total_send_data.size(), 0))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    char recv_buf[512] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(clientfd);
        char errtxt[1024] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }
    close(clientfd);
}