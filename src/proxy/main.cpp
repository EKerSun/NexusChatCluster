#include "proxyserver.hpp"
#include "rpcapplication.h"

int main(int argc, char **argv)
{
    RpcApplication::Init(argc, argv);
    ProxyServer server;
    server.Start();
    return 0;
}