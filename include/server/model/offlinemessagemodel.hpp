/**
 * @brief 存储离线消息，包括好友消息和群组消息的插入，查询，删除
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include "user.pb.h"

// 提供离线消息表的操作接口方法
class OfflineMsgModel
{
public:
    // 存储用户的离线消息
    bool Insert(int userid, int from_id, const std::string &msg);

    // 删除用户的离线消息
    void Remove(int userid);

    // 查询用户的离线消息
    void Query(int userid, UserProto::OfflineMessageList *offline_messages);

    // 存储群聊信息
    bool InsertGroupMsg(int from_id, int group_id, const std::string &msg);

    // 查询群聊信息
    bool QueryGroupMsg(UserProto::GroupList *group_list, const std::string &last_offline_time, UserProto::OfflineMessageList *offline_messages);
};

#endif