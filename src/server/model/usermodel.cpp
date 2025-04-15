#include "usermodel.hpp"
#include "db.hpp"

// 检查用户是否存在
bool UserModel::UserIsExist(int user_id)
{
    MySQL mysql;
    bool ret = false;

    char sql[1024] = {0};
    sprintf(sql, "select * from user where user_id = %d", user_id);

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

// User表的增加方法
bool UserModel::Insert(UserProto::User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.user_name().c_str(), user.user_password().c_str(), user.user_state().c_str());
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            user.set_user_id(mysql_insert_id(mysql.getConn()));
            ret = true;
        }
    }
    return ret;
}

bool UserModel::Query(int user_id, UserProto::User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where user_id = %d", user_id);
    MySQL mysql;
    bool ret = false;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                user.set_user_id(atoi(row[0]));
                user.set_user_name(row[1]);
                user.set_user_state(row[2]);
                row[3] == nullptr ? user.set_last_offline_time("") : user.set_last_offline_time(row[3]);
                user.set_user_password(row[4]);
                ret = true;
            }
            mysql_free_result(res);
        }
    }
    return ret;
}

bool UserModel::UpdateState(UserProto::User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s', last_offline_time = NOW() where user_id = %d",
            user.user_state().c_str(), user.user_id());
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
bool UserModel::ResetState()
{
    char sql[1024] = "update user set state = 'offline'";
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

