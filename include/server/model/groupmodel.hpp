/**
 * @brief 实现对群组的业务逻辑，包括群组创建、群组添加成员、群组删除成员等
 * @author EkerSun
 * @date 2025-4-15
 */

#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "user.pb.h"
#include <string>

// 维护群组信息的操作接口方法
class GroupModel
{
public:
    // 判断群组是否存在
    bool GroupIsExist(int group_id);
    // 创建群组
    bool CreateGroup(UserProto::Group &group);
    // 加入群组
    bool AddGroup(int user_id, int group_id, std::string role);
    // 退出群组
    bool QuitGroup(int user_id, int group_id);
    // 删除群组
    bool DeleteGroup(int group_id);
    // 查询用户权限
    bool QueryGroupRole(int user_id, int group_id, std::string &role);
    // 根据群id查询群
    bool QueryGroup(int group_id, UserProto::Group *);
    // 查询是否在群组中
    bool IsInGroup(int user_id, int group_id);
    // 查询用户所在群组信息
    void QueryGroups(int user_id, UserProto::GroupList *);
    // 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其它成员群发消息
    bool QueryGroupUsers(int user_id, int group_id, UserProto::GroupUserList *);
};

#endif