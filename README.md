# TCP Server

一个基于 Linux epoll 的高性能 TCP 后台服务器系统，使用 C++11 实现。

## 特性

- ✅ **高并发处理**: 使用 epoll 边缘触发模式，无需为每个客户端创建线程
- ✅ **线程池处理**: 使用 C++11 线程池处理接收到的完整消息，提高并发处理能力
- ✅ **Session 管理**: 每个客户端连接对应一个 Session 对象
- ✅ **完整粘包处理**: 完整的消息协议和粘包处理机制，支持可变长度消息体
- ✅ **严格头部验证**: 多层次验证消息头部的有效性（魔术字、类型、长度一致性）
- ✅ **身份验证**: 客户端需要登录后才能正常通信
- ✅ **心跳机制**: 10秒超时检测，自动清理失联客户端
- ✅ **消息广播**: 支持向所有已登录用户发送消息
- ✅ **单播消息**: 支持向指定用户发送消息
- ✅ **智能指针管理**: 所有对象使用智能指针自动管理内存
- ✅ **单一职责**: 每个类职责明确，通过组合方式实现功能
- ✅ **异常处理**: 线程池任务执行包含完整的异常捕获和处理

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
│   ├── ThreadPool.h            # 线程池
│   ├── EpollServer.h           # Epoll 事件循环
│   └── Server.h                # 服务器主类
├── src/                        # 源文件目录
│   ├── PacketBuffer.cpp
│   ├── Session.cpp
│   ├── SessionManager.cpp
│   ├── HeartbeatManager.cpp
│   ├── MessageDispatcher.cpp
│   ├── ThreadPool.cpp
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
   - 提供完整的头部验证函数
   - 支持粘包处理和可变长度消息体

2. **PacketBuffer (缓冲区)**
   - 处理 TCP 粘包问题
   - 从数据流中提取完整消息
   - 使用 `validateHeader()` 函数进行多层次验证
   - 验证魔术字、消息类型、长度一致性

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

7. **ThreadPool (线程池)**
   - C++11 实现的线程池
   - 处理接收到的完整消息
   - 避免阻塞网络 I/O 线程
   - 包含异常处理机制

8. **EpollServer (网络层)**
   - 基于 epoll 的事件循环
   - 非阻塞 I/O 处理
   - 管理连接、读写、断开事件
   - 提取完整消息后提交给线程池

9. **Server (服务器主类)**
   - 组合所有组件
   - 提供统一的服务器接口
   - 启动心跳检测线程
   - 管理线程池生命周期

### 消息协议

```cpp
struct MessageHeader {
    uint32_t magic;        // 魔术字 (0x12345678)
    uint16_t type;         // 消息类型
    uint16_t reserved;     // 保留字段
    uint32_t totalLength;  // 总长度 (头部 + 数据)
    uint32_t bodyLength;   // 数据长度 (可变)
};
```

**消息类型:**
- `LOGIN_REQUEST = 1` - 登录请求
- `LOGIN_RESPONSE = 2` - 登录响应
- `HEARTBEAT = 3` - 心跳包
- `DATA = 4` - 数据消息
- `BROADCAST = 5` - 广播消息

**头部验证:**

服务器对每个接收到的消息头进行严格验证：

1. **魔术字验证**: 检查 `magic == 0x12345678`
2. **类型验证**: 检查消息类型在有效范围内 (1-100)
3. **总长度验证**: 检查 `totalLength` 在合理范围内 (>= 头部大小 && <= 16MB)
4. **数据长度验证**: 检查 `bodyLength` 不超过最大限制
5. **长度一致性验证**: 检查 `totalLength == sizeof(MessageHeader) + bodyLength`

任何验证失败都会清空缓冲区并拒绝该消息。

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
# 默认端口 8888，线程池大小 4
./tcp_server

# 指定端口
./tcp_server 9999

# 指定端口和线程池大小
./tcp_server 9999 8
```

**参数说明:**
- 第一个参数: 端口号 (1-65535)，默认 8888
- 第二个参数: 线程池大小，默认 4

**启动信息示例:**
```
Starting TCP Server...
  Port: 8888
  Thread Pool Size: 4
  Heartbeat Timeout: 10 seconds
Creating thread pool with 4 threads
Server initialized with thread pool size: 4
Listening on port 8888
Server started successfully
Server running, press Ctrl+C to stop
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

`PacketBuffer` 类实现了完整的粘包处理，支持可变长度的消息体：

1. **数据累积**: 将接收到的所有数据追加到内部缓冲区
2. **头部解析**: 尝试从缓冲区读取消息头（16字节）
3. **头部验证**: 使用 `validateHeader()` 进行多层次验证
   - 验证魔术字 (0x12345678)
   - 验证消息类型 (1-100)
   - 验证总长度 (>= 16 && <= 16MB)
   - 验证数据长度 (<= 16MB - 16)
   - 验证长度一致性 (totalLength == headerSize + bodyLength)
4. **完整性检查**: 根据 `totalLength` 判断是否收到完整消息
5. **消息提取**: 提取完整消息并从缓冲区移除
6. **错误处理**: 验证失败时清空缓冲区，防止后续错误

**示例流程:**
```
接收数据: [Header1][Body1 部分]
  -> 缓冲区不足，等待更多数据

接收数据: [Body1 剩余][Header2][Body2]
  -> 提取完整的 Message1
  -> 缓冲区保留 [Header2][Body2]
  -> 提取完整的 Message2
```

### 头部有效性判断

增强的头部验证确保服务器的健壮性：

```cpp
enum class HeaderValidationResult {
    VALID,                    // 有效
    INVALID_MAGIC,           // 魔术字错误
    INVALID_TYPE,            // 类型错误
    INVALID_TOTAL_LENGTH,    // 总长度错误
    INVALID_BODY_LENGTH,     // 数据长度错误
    LENGTH_MISMATCH          // 长度不一致
};
```

**验证失败时的处理:**
- 打印详细的错误信息（魔术字、类型、长度）
- 清空整个接收缓冲区
- 防止恶意或损坏的数据影响后续处理

### 线程池处理

当 `EpollServer` 从网络接收并提取出完整消息后，不会在 I/O 线程中直接处理，而是提交到线程池：

**处理流程:**
1. **I/O 线程** (epoll): 接收数据 → 粘包处理 → 提取完整消息
2. **提交任务**: 将完整消息（header + body）提交到线程池
3. **工作线程**: 从任务队列获取任务 → 调用 `MessageDispatcher` → 业务处理
4. **异常处理**: 捕获并记录工作线程中的所有异常

**优势:**
- **I/O 线程不阻塞**: 网络接收和消息处理分离
- **并发处理**: 多个消息可以同时在不同线程中处理
- **可扩展性**: 可以根据 CPU 核心数调整线程池大小
- **异常隔离**: 单个消息处理异常不影响其他消息

**线程池配置:**
```bash
# 使用默认线程数 (4)
./tcp_server 8888

# 使用 8 个工作线程
./tcp_server 8888 8

# 根据 CPU 核心数自动调整
./tcp_server 8888 $(nproc)
```

### 心跳超时

- 客户端需要每 10 秒内至少发送一次心跳
- 服务器每秒检查一次所有已认证会话
- 超时的会话会被自动关闭并清理

### 高并发处理

- 使用 epoll 边缘触发模式
- 非阻塞 I/O
- 单线程处理所有网络事件
- 独立线程进行心跳检测
- 线程池处理业务逻辑

### 线程安全

- `SessionManager` 使用互斥锁保护会话集合
- `ThreadPool` 使用条件变量和互斥锁管理任务队列
- 心跳检测在独立线程中运行
- 消息处理在线程池中并发执行

## 扩展建议

1. **数据库集成**: 将用户认证信息存储在数据库中
2. **加密通信**: 添加 SSL/TLS 支持
3. **消息队列**: 集成消息队列处理异步任务
4. **日志系统**: 添加完善的日志记录
5. **配置文件**: 支持从配置文件读取参数
6. **性能监控**: 添加性能指标收集和监控

## 许可证

MIT License
