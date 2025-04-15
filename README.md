# NexusChatCluster
​**​NexusChatCluster​**​ 是一个基于分布式架构的聊天服务器，支持高并发、低延迟消息传输，通过 Nginx 实现负载均衡，结合服务发现机制动态管理网关与微服务节点。
## 核心功能
- 代理服务器和微服务后端都是基于muduo网络库，利用IO复用技术Epoll与线程池实现多线程的Reactor高并发TCP服务端；
- 微服务后端启动后，在zookeeper注册服务和方法节点
- 代理服务器，解析客户端请求，并连接zookeeper服务器，发现已经注册的服务节点，通过RPC远程服务调用处理具体业务；
- 客户端使用分离线程接收消息，避免阻塞等待；
- 配置nginx基于TCP的负载均衡，实现聊天服务器的集群功能，将客户端请求负载到代理服务器，对外只暴露nginx监听端口，提高安全性；
- 基于Protobuf设计远程服务调用和服务请求的通信协议；
- 基于redis的发布订-阅功能，设计服务器间的消息通信，实现实时跨服务器的业务消息传输；
- 使用mysql存储项目数据，设计了用户表、群组表、好友关系表、群组用户表、离线消息表、群组消息表。
- 实现业务：登录、注册、退出、创建群、删除群、加入群、退出群、加好友、删除好友、实时一对一聊天、实时群组聊天、离线消息存储和抓取、好友和群组信息抓取

## 项目目录
```
.
├── CMakeLists.txt
├── bin              # 项目文件输出路径
│   ├── TheClient
│   ├── TheGate
│   ├── TheServer
│   ├── test.conf
│   └── test2.conf
├── build
│   └── Makefile
├── chat.sql         # sql脚本
├── include
│   ├── client                     # 客户端
│   │   ├── chatclient.hpp
│   │   └── clientservice.hpp
│   ├── proxy                      # 代理服务器
│   │   ├── proxyserver.hpp
│   │   └── proxyservice.hpp
│   ├── public.hpp
│   ├── server                     # 微服务后端
│   │   ├── chatserver.hpp
│   │   ├── chatservice.hpp
│   │   ├── db
│   │   │   └── db.hpp
│   │   ├── model
│   │   │   ├── friendmodel.hpp
│   │   │   ├── groupmodel.hpp
│   │   │   ├── offlinemessagemodel.hpp
│   │   │   └── usermodel.hpp
│   │   └── redis
│   │       └── redis.hpp
│   └── user.pb.h
├── lib
│   └── librpcserver.a
├── rpc                             # RPC远程服务调用框架
│   ├── CMakeLists.txt
│   ├── include
│   │   ├── rpcapplication.h
│   │   ├── rpcchannel.h
│   │   ├── rpcconfig.h
│   │   ├── rpccontroller.h
│   │   ├── rpcheader.pb.h
│   │   ├── rpcprovider.h
│   │   └── zookeeperutil.h
│   ├── rpcapplication.cc
│   ├── rpcchannel.cc
│   ├── rpcconfig.cc
│   ├── rpccontroller.cc
│   ├── rpcheader.pb.cc
│   ├── rpcprovider.cc
│   └── zookeeperutil.cc
├── rpcheader.proto
├── src
│   ├── CMakeLists.txt
│   ├── client
│   │   ├── CMakeLists.txt
│   │   ├── chatclient.cpp
│   │   ├── clientservice.cpp
│   │   └── main.cpp
│   ├── proxy
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   ├── proxyserver.cpp
│   │   └── proxyservice.cpp
│   ├── server
│   │   ├── CMakeLists.txt
│   │   ├── chatserver.cpp
│   │   ├── chatservice.cpp
│   │   ├── db
│   │   │   └── db.cpp
│   │   ├── main.cpp
│   │   ├── model
│   │   │   ├── friendmodel.cpp
│   │   │   ├── groupmodel.cpp
│   │   │   ├── offlinemessagemodel.cpp
│   │   │   └── usermodel.cpp
│   │   └── redis
│   │       └── redis.cpp
│   └── user.pb.cc
└── user.proto
```

## 快速开始
### 运行环境
- Ubuntu-24.04
- C++17
### 前置依赖
- muduo
- mysql
- redis
- nginx
- zookeeper
- protobuf
### 项目启动
- 数据库配置
```mysql
# 创建数据库
CREATE DATABASE chat;
# 切换到chat表
USE chat;
# 执行sql脚本文件，生成所需数据库
source chat.sql;
# 需要先修改src/server/db/db.cpp的user和password,改成自己的Mysql登录信息
```
- 配置nginx
```bash
# 修改nginx配置文件(Ubuntu)
# 参考提供的nginx.conf配置文件修改stream的负载端口和监听端口
sudo vi /etc/nginx/nginx.conf
nginx -t
nginx -s reload
```
- 启动服务端
```bash
cd bin
./TheServer -i test.conf
```
- 启动代理服务器
```bash
cd bin
./TheGate -i test.conf
```
- 启动客户端
```bash
cd bin
# 假设nginx监听8000端口
./TheClient 127.0.0.1 8000
```
- 模拟集群服务器，配置多zookeeper服务器实例，不模拟集群可跳过
```bash
# 创建多个 ZooKeeper 实例的配置目录
mkdir -p /opt/zookeeper/zk1/conf
mkdir -p /opt/zookeeper/zk2/conf
# 复制默认配置文件并修改， ZKCONF_PATH 为zookeeper配置文件路径
cp ${ZKCONF_PATH}/zoo.cfg opt/zookeeper/zk1/conf/zoo.cfg
cp ${ZKCONF_PATH}/zoo.cfg opt/zookeeper/zk2/conf/zoo.cfg
# 针对每个实例 以zk1为例
mkdir /opt/zookeeper/zk1/data
vi /opt/zookeeper/zk1/zoo.cfg
# 修改客户端端口和数据目录
# clientPort=2182， 与zookeeper默认端口2181不同
# dataDir=/opt/zookeeper/zk1/data
# 启动实例，ZK_PATH为zookeeper安装路径
${ZK_PATH}/bin/zkServer.sh start /opt/zookeeper/zk1/conf/zoo.cfg
```
## 后续工作
- 通信安全
- 缓存数据库
- 文件传输和视频通话
- UI

