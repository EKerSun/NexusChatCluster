/**
 * @brief 定义消息类型，供protobuf数据头使用
 * @author EkerSun
 * @date 2025-2-20
 */

#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    REG_MSG = 1, // 注册消息
    LOGIN_MSG,   // 登录消息
    QUIT_MSG,    // 退出消息

    ADD_FRIEND_MSG,    // 添加好友信息
    DELETE_FRIEND_MSG, // 删除好友信息

    CREATE_GROUP_MSG, // 创建群组
    DELETE_GROUP_MSG, // 删除群组
    ADD_GROUP_MSG,    // 加入群组
    QUIT_GROUP_MSG,   // 退出群组

    FRIEND_CHAT_MSG, // 好友聊天消息
    GROUP_CHAT_MSG,  // 群组聊天消息

    LOGOUT_MSG, // 登出

    RECV_FRIEND_MSG, // 收到好友消息
    RECV_GROUP_MSG,  // 收到群组消息
    RECV_ADD_FRIEND_MSG,    // 收到添加好友消息
    RECV_DELETE_FRIEND_MSG, // 收到删除好友消息
    RECV_ADD_GROUP_MSG,     // 收到加入群组消息
    RECV_QUIT_GROUP_MSG,    // 收到退出群组消息
    RECV_DELETE_GROUP_MSG,  // 收到删除群组消息


    REG_MSG_ACK,   // 注册响应消息
    LOGIN_MSG_ACK, // 登录响应消息

    ADD_FRIEND_MSG_ACK,    // 添加好友响应
    DELETE_FRIEND_MSG_ACK, // 删除好友响应

    CREATE_GROUP_MSG_ACK, // 创建群组响应
    DELETE_GROUP_MSG_ACK, // 删除群组响应
    ADD_GROUP_MSG_ACK,    // 加入群组响应
    QUIT_GROUP_MSG_ACK,   // 退出群组响应

    FRIEND_CHAT_MSG_ACK, // 好友聊天响应
    GROUP_CHAT_MSG_ACK,  // 群组聊天响应

    LOGOUT_MSG_ACK, // 登出响应

};

#endif