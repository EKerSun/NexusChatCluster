
#include "chatserver.hpp"
#include "rpcapplication.h"
int main(int argc, char **argv)
{
    RpcApplication::Init(argc, argv);
    ChatServer server;
    server.Start();
    return 0;
}
