-- Active: 1741265566877@@127.0.0.1@3306@chat

-- 用户表
DROP TABLE IF EXISTS `group_message`;
DROP TABLE IF EXISTS `offlinemessage`;
DROP TABLE IF EXISTS `user_friend`;
DROP TABLE IF EXISTS `group_user`;
DROP TABLE IF EXISTS `group`;
DROP TABLE IF EXISTS `user`;


CREATE TABLE `user` (
  `user_id` INT AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID',
  `name` VARCHAR(50) NOT NULL UNIQUE COMMENT '姓名',
  `state` ENUM('online','offline') DEFAULT 'offline' COMMENT '状态',
    `last_offline_time` DATETIME COMMENT '上次离线时间',
  `password` VARCHAR(100) NOT NULL COMMENT '密码（哈希值，推荐使用 bcrypt 或 Argon2）',
  `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 好友关系表

CREATE TABLE `user_friend` (
  `user_id1` INT NOT NULL COMMENT '用户ID',
  `user_id2` INT NOT NULL COMMENT '好友ID',
  `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '添加时间',
  PRIMARY KEY (`user_id1`, `user_id2`), -- 联合主键防止重复
  FOREIGN KEY (`user_id1`) 
    REFERENCES `user`(`user_id`)
    ON DELETE CASCADE, -- 用户删除时级联删除好友关系
  FOREIGN KEY (`user_id2`) 
    REFERENCES `user`(`user_id`)
    ON DELETE CASCADE,
  CHECK (`user_id1` < `user_id2`) -- 确保单向存储（避免重复）
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 离线消息表
CREATE TABLE `offlinemessage` (
  `message_id` INT AUTO_INCREMENT PRIMARY KEY COMMENT '消息ID',
  `user_id` INT NOT NULL COMMENT '用户ID',
  `from_id` INT NOT NULL COMMENT '来自ID',
  `message` VARCHAR(1024) NOT NULL COMMENT '消息内容',
  `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
  UNIQUE KEY (`user_id`, `message`(100)) COMMENT '防止重复消息',
  FOREIGN KEY (`user_id`) 
    REFERENCES `user`(`user_id`)
    ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 群组表
CREATE TABLE `group` (
  `group_id` INT AUTO_INCREMENT PRIMARY KEY COMMENT '群ID',
  `group_name` VARCHAR(50) NOT NULL COMMENT '群名称',
  `group_desc` VARCHAR(50) COMMENT '群描述',
  `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群组用户关系表
CREATE TABLE `group_user` (
  `group_id` INT NOT NULL COMMENT '群ID',
  `user_id` INT NOT NULL COMMENT '用户ID',
  `role` ENUM('admin', 'member') DEFAULT 'member' COMMENT '角色',
  `join_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
  PRIMARY KEY (`group_id`, `user_id`), -- 联合主键
  FOREIGN KEY (`group_id`) 
    REFERENCES `group`(`group_id`)
    ON DELETE CASCADE, -- 级联删除
  FOREIGN KEY (`user_id`) 
    REFERENCES `user`(`user_id`)
    ON DELETE CASCADE -- 用户删除时自动退出所有群组
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `group_message` (
  `message_id` INT AUTO_INCREMENT PRIMARY KEY COMMENT '消息ID',
  `group_id` INT NOT NULL COMMENT '群ID',
  `from_id` INT NOT NULL COMMENT '来自ID',
  `message` VARCHAR(1024) NOT NULL COMMENT '消息内容',
  `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
  FOREIGN KEY (`group_id`)
    REFERENCES `group`(`group_id`)
    ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



