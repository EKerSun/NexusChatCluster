#include "chatserver.hpp"
#include <iostream>
void ChatServer::Start()
{
    ChatService::Instance()->Init();
    service_library_ = ChatService::Instance()->GetServiceLibrary();
    keep_alive_method_set_ = ChatService::Instance()->GetKeepAliveMethodSet();
    provider_.NotifyService(service_library_, keep_alive_method_set_);
    provider_.Run();
}