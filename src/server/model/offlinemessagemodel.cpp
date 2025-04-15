#include "offlinemessagemodel.hpp"
#include "db.hpp"

// 存储群聊信息
bool OfflineMsgModel::InsertGroupMsg(int from_id, int group_id, const std::string &msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into group_message (from_id, group_id, message) values(%d, %d, '%s')", from_id, group_id, msg.c_str());
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        ret = mysql.update(sql);
    }
    return ret;
}

bool OfflineMsgModel::QueryGroupMsg(UserProto::GroupList *group_list, const std::string &last_offline_time, UserProto::OfflineMessageList *offlien_message_list)
{
    // 1.组装sql语句
    std::string group_ids;
    for (auto &group : group_list->groups())
    {
        group_ids += std::to_string(group.group_id());
        group_ids.push_back(',');
    }
    group_ids.pop_back();
    char sql[1024] = {0};
    sprintf(sql, "select group_id, from_id  message, create_time from group_message where group_id in (%s) and create_time > '%s'", group_ids.c_str(), last_offline_time.c_str());
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                UserProto::Message *message = offlien_message_list->add_messages();
                message->mutable_receiver()->set_group_id(atoi(row[0]));
                message->mutable_sender()->set_user_id(atoi(row[1]));
                message->set_content(row[2]);
                message->set_time(row[3]);
                message->set_message_type(UserProto::MessageType::GROUP_CHAT);
                ret = true;
            }
            mysql_free_result(res);
        }
    }
    return ret;
}

// 存储用户的离线消息
bool OfflineMsgModel::Insert(int user_id, int from_id, const std::string &msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage (user_id, from_id, message) values(%d, %d, '%s')", user_id, from_id, msg.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return true;
    }
    return false;
}

// 删除用户的离线消息
void OfflineMsgModel::Remove(int user_id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where user_id=%d", user_id);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
void OfflineMsgModel::Query(int user_id, UserProto::OfflineMessageList *offlien_message_list)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select from_id, message, create_time from offlinemessage where user_id = %d", user_id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                UserProto::Message *message = offlien_message_list->add_messages();
                message->mutable_sender()->set_user_id(atoi(row[0]));
                message->set_content(row[1]);
                message->set_time(row[2]);
                message->set_message_type(UserProto::MessageType::FRIEND_CHAT);
            }
            mysql_free_result(res);
        }
    }
}