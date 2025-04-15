#include "friendmodel.hpp"
#include "db.hpp"
bool FriendModel::IsFriend(int user_id, int friend_id)
{
    // 查询好友关系
    int max_id = std::max(user_id, friend_id);
    int min_id = std::min(user_id, friend_id);
    char sql[1024] = {0};
    sprintf(sql, "select user_id1 from user_friend where user_id1 = %d and user_id2 = %d", min_id, max_id);
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            ret = (row != nullptr);
            mysql_free_result(res);
        }
    }
    return ret;
}

// 添加好友关系
bool FriendModel::Insert(int user_id, int friend_id)
{
    // 检查用户是否存在
    int max_id = std::max(user_id, friend_id);
    int min_id = std::min(user_id, friend_id);
    char sql[1024] = {0};
    sprintf(sql, "select user_id from user where user_id = %d", min_id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res && mysql_num_rows(res) == 0)
        {
            mysql_free_result(res);
            sprintf(sql, "insert into user_friend (user_id1, user_id2) values (%d, %d)", min_id, max_id);
            if (mysql.update(sql))
            {
                return true;
            }
        }
    }
    return false;
}
// 删除好友
bool FriendModel::Delete(int user_id, int friend_id)
{
    // 合法性检查
    if (user_id < 0 || friend_id < 0)
        return false;
    if (user_id == friend_id)
        return false;
    // 查询好友关系
    int max_id = std::max(user_id, friend_id);
    int min_id = std::min(user_id, friend_id);
    char sql[1024] = {0};
    sprintf(sql, "select user_id1 from user_friend where user_id1 = %d and user_id2 = %d", min_id, max_id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res && mysql_num_rows(res) == 0)
        {
            mysql_free_result(res);
            sprintf(sql, "delete from user_friend where user_id1 = %d and user_id2 = %d", min_id, max_id);
            if (mysql.update(sql))
            {
                return true;
            }
        }
    }
    return false;
}

// 返回用户好友列表
void FriendModel::Query(int user_id, UserProto::FriendList *friend_list)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.user_id, a.name, a.state from user a inner join user_friend b on case\
            when b.user_id1 = %d then b.user_id2 = a.user_id \
            when b.user_id2 = %d then b.user_id1 = a.user_id \
            end",
            user_id, user_id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                UserProto::FriendUser *friend_user = friend_list->add_friend_users();
                friend_user->set_friend_id(atoi(row[0]));
                friend_user->set_friend_name(row[1]);
            }
            mysql_free_result(res);
        }
    }
}