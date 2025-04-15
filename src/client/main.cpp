#include <iostream>
#include "chatclient.hpp"
using namespace std;
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 7001" << endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的ip和port
    string ip = argv[1];
    u_int16_t port = atoi(argv[2]);
    ChatClient client(ip.c_str(), port);
    client.Start();
}