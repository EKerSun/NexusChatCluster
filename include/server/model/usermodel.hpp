
/**
 * @brief 描述用户表的维护方法，涉及用户表的插入、查询、更新
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.pb.h"
// user表的数据操作类
class UserModel
{
public:
    // 检查用户是否存在
    bool UserIsExist(int user_id);
    // User表的增加方法
    bool Insert(UserProto::User &user);
    // 根据用户号码查询用户信息
    bool Query(int user_id, UserProto::User &user);
    // 更新用户的状态信息
    bool UpdateState(UserProto::User &user);
    // 重置用户的状态信息
    bool ResetState();
};

#endif