/**
 * @brief 实现对好友表中插入、删除和查询操作
 * @author EkerSun
 * @date 2025-4-15
 */
#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include "user.pb.h"
class FriendModel
{
public:
    // 查询是否已经是好友关系
    bool IsFriend(int userid, int friendid);
    // 添加好友关系
    bool Insert(int userid, int friendid);
    // 删除好友关系
    bool Delete(int userid, int friendid);
    // 返回用户好友列表
    void Query(int userid, UserProto::FriendList *);

private:
};
#endif