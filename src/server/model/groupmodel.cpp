#include "groupmodel.hpp"
#include "db.hpp"

// 群组是否存在
bool GroupModel::GroupIsExist(int group_id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select group_id from `group` where group_id = %d", group_id);
    MySQL mysql;
    int ret = false;
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

// 创建群组
bool GroupModel::CreateGroup(UserProto::Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into `group` (group_name, group_desc) values('%s', '%s')",
            group.group_name().c_str(), group.group_description().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.set_group_id(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;
}

// 加入群组
bool GroupModel::AddGroup(int user_id, int group_id, std::string role)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into group_user (group_id, user_id, role) values(%d, %d, '%s')", group_id, user_id, role.c_str());
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            ret = true;
        }
    }
    return ret;
}

// 退出群组
bool GroupModel::QuitGroup(int user_id, int group_id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from group_user where user_id = %d and group_id = %d", user_id, group_id);
    MySQL mysql;
    int ret = false;
    if (mysql.connect())
    {
        if (mysql.update(sql))
            ret = true;
    }
    return ret;
}

// 删除群组
bool GroupModel::DeleteGroup(int group_id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from `group` where group_id = %d", group_id);
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
            return true;
    }
    return false;
}

// 查询用户权限
bool GroupModel::QueryGroupRole(int user_id, int group_id, std::string &role)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select role from group_user where user_id = %d and group_id = %d", user_id, group_id);
    MySQL mysql;
    int ret = false;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                role = row[0];
                ret = true;
            }
            mysql_free_result(res);
        }
    }
    return ret;
}

bool GroupModel::QueryGroup(int group_id, UserProto::Group *group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select group_id, group_name, group_desc from `group` where group_id = %d", group_id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                group->set_group_id(atoi(row[0]));
                group->set_group_name(row[1]);
                group->set_group_description(row[2]);
            }
            mysql_free_result(res);
            return true;
        }
    }
    return false;
}

// 判断该用户是否在群组中
bool GroupModel::IsInGroup(int user_id, int group_id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select user_id from group_user where user_id = %d and group_id = %d", user_id, group_id);
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (mysql_fetch_row(res) != nullptr)
        {
            mysql_free_result(res);
            return ret = true;
        }
    }
    return ret;
}

// 查询用户所在群组信息
void GroupModel::QueryGroups(int user_id, UserProto::GroupList *group_list)
{
    /*
    1. 先根据user_id在groupuser表中查询出该用户所属的群组信息
    2. 在根据群组信息，查询属于该群组的所有用户的user_id，并且和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.group_id, a.group_name, a.group_desc from `group` a inner join \
            group_user b on a.group_id = b.group_id where b.user_id=%d",
            user_id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            // 查出user_id所有的群组信息
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                UserProto::Group *group = group_list->add_groups();
                group->set_group_id(atoi(row[0]));
                group->set_group_name(row[1]);
                group->set_group_description(row[2]);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (UserProto::Group group : group_list->groups())
    {
        sprintf(sql, "select a.user_id,a.name,a.state,b.role from user a \
                inner join group_user b on b.user_id = a.user_id where b.group_id = %d",
                group.group_id());
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                UserProto::GroupUser *group_user = group.mutable_group_user_list()->add_group_users();
                group_user->set_user_id(atoi(row[0]));
                group_user->set_user_name(row[1]);
                group_user->set_user_state(row[2]);
                group_user->set_user_role(row[3]);
            }
            mysql_free_result(res);
        }
    }
}

// 根据指定的group_id查询群组用户id列表，除user_id自己，主要用户群聊业务给群组其它成员群发消息
bool GroupModel::QueryGroupUsers(int user_id, int group_id, UserProto::GroupUserList *group_user_list)
{
    char sql[1024] = {0};
    sprintf(sql, "select user_id from group_user where group_id = %d and user_id != %d", group_id, user_id);
    MySQL mysql;
    int ret = false;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                group_user_list->add_group_users()->set_user_id(atoi(row[0]));
            }
            ret = true;
            mysql_free_result(res);
        }
    }
    return ret;
}