# TCP Server

一个基于 Linux epoll 的高性能 TCP 后台服务器系统，使用 C++11 实现。

## 特性

- ✅ **高并发处理**: 使用 epoll 边缘触发模式，无需为每个客户端创建线程
- ✅ **Session 管理**: 每个客户端连接对应一个 Session 对象
- ✅ **粘包处理**: 完整的消息协议和粘包处理机制
- ✅ **身份验证**: 客户端需要登录后才能正常通信
- ✅ **心跳机制**: 10秒超时检测，自动清理失联客户端
- ✅ **消息广播**: 支持向所有已登录用户发送消息
- ✅ **单播消息**: 支持向指定用户发送消息
- ✅ **智能指针管理**: 所有对象使用智能指针自动管理内存
- ✅ **单一职责**: 每个类职责明确，通过组合方式实现功能

## 项目结构

```
tcp_server/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 项目说明文档
├── include/                    # 头文件目录
│   ├── Protocol.h              # 消息协议定义
│   ├── PacketBuffer.h          # 粘包处理缓冲区
│   ├── Session.h               # 客户端会话
│   ├── SessionManager.h        # 会话管理器
│   ├── HeartbeatManager.h      # 心跳管理器
│   ├── MessageDispatcher.h     # 消息分发器
│   ├── EpollServer.h           # Epoll 事件循环
│   └── Server.h                # 服务器主类
├── src/                        # 源文件目录
│   ├── PacketBuffer.cpp
│   ├── Session.cpp
│   ├── SessionManager.cpp
│   ├── HeartbeatManager.cpp
│   ├── MessageDispatcher.cpp
│   ├── EpollServer.cpp
│   ├── Server.cpp
│   └── main.cpp                # 程序入口
└── test/                       # 测试目录
    └── test_client.cpp         # 测试客户端
```

## 架构设计

### 核心组件

1. **Protocol (协议层)**
   - 定义消息头结构
   - 包含魔术字、消息类型、长度字段
   - 支持粘包处理

2. **PacketBuffer (缓冲区)**
   - 处理 TCP 粘包问题
   - 从数据流中提取完整消息
   - 验证消息完整性

3. **Session (会话)**
   - 代表单个客户端连接
   - 管理连接状态和认证状态
   - 记录最后心跳时间

4. **SessionManager (会话管理器)**
   - 管理所有客户端会话
   - 提供广播和单播接口
   - 线程安全的会话操作

5. **HeartbeatManager (心跳管理器)**
   - 监控客户端心跳
   - 检测超时连接
   - 返回失联客户端列表

6. **MessageDispatcher (消息分发器)**
   - 根据消息类型路由到对应处理器
   - 处理登录、心跳、数据消息
   - 业务逻辑处理

7. **EpollServer (网络层)**
   - 基于 epoll 的事件循环
   - 非阻塞 I/O 处理
   - 管理连接、读写、断开事件

8. **Server (服务器主类)**
   - 组合所有组件
   - 提供统一的服务器接口
   - 启动心跳检测线程

### 消息协议

```cpp
struct MessageHeader {
    uint32_t magic;        // 魔术字 (0x12345678)
    uint16_t type;         // 消息类型
    uint16_t reserved;     // 保留字段
    uint32_t totalLength;  // 总长度 (头部 + 数据)
    uint32_t bodyLength;   // 数据长度
};
```

**消息类型:**
- `LOGIN_REQUEST = 1` - 登录请求
- `LOGIN_RESPONSE = 2` - 登录响应
- `HEARTBEAT = 3` - 心跳包
- `DATA = 4` - 数据消息
- `BROADCAST = 5` - 广播消息

## 编译

### 前置要求

- Linux 操作系统
- GCC 4.8+ (支持 C++11)
- CMake 3.10+

### 编译步骤

```bash
cd tcp_server
mkdir build && cd build
cmake ..
make
```

编译成功后会生成 `tcp_server` 可执行文件。

### 编译测试客户端

```bash
# 在 build 目录中
g++ -std=c++11 -I../include ../test/test_client.cpp -o test_client -lpthread
```

## 运行

### 启动服务器

```bash
# 默认端口 8888
./tcp_server

# 指定端口
./tcp_server 9999
```

### 运行测试客户端

```bash
# 连接到本地服务器
./test_client

# 连接到指定服务器
./test_client 192.168.1.100 8888
```

## 使用示例

### 客户端连接流程

1. **建立 TCP 连接**
2. **发送登录请求**
   ```cpp
   LoginRequest req;
   strcpy(req.username, "user1");
   strcpy(req.password, "pass123");
   // 发送带有 LOGIN_REQUEST 类型的消息
   ```

3. **接收登录响应**
   ```cpp
   LoginResponse resp;
   // 接收 LOGIN_RESPONSE 消息
   if (resp.success == 1) {
       // 登录成功
   }
   ```

4. **定期发送心跳**
   ```cpp
   // 每 5 秒发送一次心跳
   MessageHeader header;
   header.type = HEARTBEAT;
   header.bodyLength = 0;
   header.totalLength = sizeof(MessageHeader);
   ```

5. **发送数据消息**
   ```cpp
   MessageHeader header;
   header.type = DATA;
   header.bodyLength = data.size();
   header.totalLength = sizeof(MessageHeader) + data.size();
   // 发送 header + data
   ```

## 关键特性说明

### 粘包处理

`PacketBuffer` 类实现了完整的粘包处理:
- 累积接收到的数据
- 根据 `totalLength` 字段判断是否收到完整消息
- 验证魔术字和长度字段
- 提取完整消息并从缓冲区移除

### 心跳超时

- 客户端需要每 10 秒内至少发送一次心跳
- 服务器每秒检查一次所有已认证会话
- 超时的会话会被自动关闭并清理

### 高并发处理

- 使用 epoll 边缘触发模式
- 非阻塞 I/O
- 单线程处理所有网络事件
- 独立线程进行心跳检测

### 线程安全

- `SessionManager` 使用互斥锁保护会话集合
- 心跳检测在独立线程中运行
- 避免了为每个连接创建线程的开销

## 扩展建议

1. **数据库集成**: 将用户认证信息存储在数据库中
2. **加密通信**: 添加 SSL/TLS 支持
3. **消息队列**: 集成消息队列处理异步任务
4. **日志系统**: 添加完善的日志记录
5. **配置文件**: 支持从配置文件读取参数
6. **性能监控**: 添加性能指标收集和监控

## 许可证

MIT License
