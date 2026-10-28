# MMIO 总线接口与背压模型

## 目标和模型边界

本版本在固定指令类别延迟的 RV64+V Atomic Hart 上，新增程序可见的 MMIO 总线控制窗口，并为 NoC 目的端增加完整消息接收额度。所有 Hart 在统一模拟时钟下逻辑并发执行，宿主程序单线程逐周期推进。

普通标量/向量访存仍然生成 NoC 请求。新增 MMIO 使用标量 LD/SD 访问本地控制窗口，启动的目标事务通过同一个 NoC 端口访问共享内存。每个 Hart 有一份独立窗口，不支持通过此窗口操作其他 Hart。没有新增自定义 CSR。

## 寄存器约定

基地址固定为 `0x90000000`，保留窗口 `[0x90000000, 0x90000100)`。

| 偏移 | 名称 | 读写规则 |
|---|---|---|
| 0x00 | TARGET_ADDR | 目标共享内存地址 |
| 0x08 | WRITE_DATA | 小端写数据，取低 SIZE 字节 |
| 0x10 | SIZE | 目标事务宽度：1/2/4/8 字节，默认8 |
| 0x18 | COMMAND | 写1发起读，写2发起写，读回0 |
| 0x20 | STATUS | bit0=BUSY、bit1=DONE、bit2=ERROR；空闲时写1清除对应DONE/ERROR位 |
| 0x28 | READ_DATA | 只读；成功读取的数据零扩展到64位 |

寄存器访问本身只允许8字节对齐的64位LD/SD；SIZE表示目标事务宽度，与寄存器访问宽度不同。未知偏移、非64位访问、向量访问控制窗口会使 Hart Fault。非法命令、非法目标宽度、地址回绕、目标范围与本地窗口重叠会设置 ERROR，不产生总线事务。共享内存是稀疏模型，其他64位目标地址均可访问，不模拟页表或总线地址保护。

空闲时写 COMMAND 开始新事务会清除上一轮 DONE/ERROR/READ_DATA。忙时写任何控制寄存器均被拒绝并设置 ERROR，原事务继续完成。BUSY清除后应先检查ERROR，再使用READ_DATA；DONE只表示已收到响应，不保证没有错误。

示例程序 `examples/mmio_bus_smoke.S` 对每个 Hart 的私有地址发起写事务，再发起读事务并核对数据。数据地址由 `0x6000 + mhartid * 256` 计算。`examples` 中的 `tohost` 仍是独立的裸机结束约定：直接 Store 收到响应后，由 Hart 解释其写入值；通过适配器向该地址写数据不触发结束。

## 端口仲裁和时序

适配器每 Hart 最多一个未完成事务，保留事务ID=0；普通访存使用从1开始递增的ID。命令通过本地Store提交后，适配器从后续tick开始尝试发送。只在普通Hart没有未完成事务时才能执行该命令指令。

适配器忙时优先持有请求/响应端口。Hart仍能执行标量计算和本地状态轮询；普通及向量数据请求等待适配器完成，避免偷取消费响应。Fence也等待适配器排空。Hart正常停止后，系统继续推进适配器，直到它与内存控制器均空闲。系统Fault仍按原规则立即报告失败。

这不是多Outstanding向量核心或DMA。它是一个单事务总线控制接口。MMIO状态轮询只访问本地寄存器，不制造额外NoC流量；目标读写与普通访存会制造NoC流量。

## NoC完整消息接收额度

`--receive-slots N` 控制每个目的端口、每个通道最多同时接纳多少条完整消息。众核默认N=2，N=0回到旧版无界接收模式，供对照实验使用。独立NoC库默认仍为0以兼容原调用方。

发送完整消息时预留目的端额度；消息在源端、链路中、目的端重组、等待应用recv期间均占用额度；只有应用recv取走完整消息才释放。每条消息最大4096字节，超限会报错，所以有限消息数量也约束了重组数据容量（不含容器开销和分包填充）。请求通道0与响应通道1独立计数。

目的端额度不足时，`send()`返回false，调用方保留完整请求并重试。`can_send()`仍只检查源端条件，因此can_send成功不保证send成功。已预留消息的全部分包允许收齐，不再对同一条消息施加可能导致重组死锁的局部小包数限制。中间转发缓冲仍使用原有水位背压。

这是简化的端到端接纳控制，假定额度检查即时可见，不模拟硬件credit返回链路延迟，也不是对任意拓扑的无死锁证明。已有测试覆盖双环众核请求/响应和小缓冲多包消息。共享内存仍为单控制器、单活动服务请求，Private与Hotspot未引入Bank相关时序差异。

## 固定延迟配置

| 参数 | 默认执行等待周期 |
|---|---:|
| --lat-alu | 1 |
| --lat-mul | 3 |
| --lat-div | 18 |
| --lat-branch | 1 |
| --lat-csr | 1 |
| --lat-memory | 1 |
| --lat-fence | 1 |
| --lat-vconfig | 1 |
| --lat-valu | 4 |
| --lat-vtransaction | 1 |

`--lat-memory`作用于标量和向量访存指令的准备阶段。`--lat-vtransaction`控制向量每个子事务的注入准备，默认1即使用原本的MemoryIssue周期，配置大于1时增加额外准备等待。它不控制NoC链路传输速度。向量ALU仍使用整条指令固定周期，不按VL/LMUL模拟Lane吞吐。

以上均为模型参数，并非测得的真实芯片周期。一条普通指令还包含一个FetchDecode周期；访存另有MemoryIssue、动态注入等待、响应等待及响应消费周期，所以固定执行延迟不是整条指令的总耗时。

## 统计的含义

- `fetch_decode`：各Hart取指译码周期之和；取指走本地ProgramImage。
- `execute_wait`：固定指令执行等待周期之和。
- `vector_issue_wait`：向量子事务额外准备周期；每次基础注入尝试仍占用一个tick。
- `injection_blocked`：普通/向量请求注入失败周期，包括源端背压或目的端无额度。
- `memory_wait`：请求发出后尚未收到响应的周期，包括网络、内存排队、服务、响应返回；不是纯内存队列等待。
- `adapter_serialization_wait`：普通访存或Fence等待本Hart MMIO适配器完成的周期。
- `mmio_*`：适配器命令数、完成数、拒绝/错误事件数、注入等待和响应等待。适配器统计可与Hart执行重叠，不能全部相加当作总周期。
- `destination_credit_stalls`：NoC send因目的端额度不足失败的次数，是调用次数，不是网络节点阻塞周期。
- `max_reserved_messages`：任何单个目的端口/通道的最大同时预留消息数。
- `memory_queued_request_cycles`：逐周期累加内存队列长度，单位为请求·周期。
- `memory_service_cycles`：内存活动请求实际服务周期。
- `memory_response_injection_wait`：内存控制器等待响应注入的周期。
- `memory_queue_full_cycles`：请求队列满并且NoC已有完整请求待取的周期。

各Hart等待周期和NoC各节点阻塞计数都是累计量，可能大于全局模拟周期。CSV schema_version=2，包含所有延迟配置和接收额度；不能追加到旧版CSV，应指定新结果文件。

## 验收命令

从仓库根目录执行：

```bash
cmake -S noc -B noc/build
cmake --build noc/build -j2
(cd noc/build && ctest --output-on-failure)
cmake -S rv64-manycore-sim -B rv64-manycore-sim/build
cmake --build rv64-manycore-sim/build -j2
(cd rv64-manycore-sim/build && ctest --output-on-failure)
bash rv64-manycore-sim/scripts/run_contention_sweep.sh \
  rv64-manycore-sim/build rv64-manycore-sim/build/results/bounded.csv
```

需要C++20编译器、CMake、bash；构建裸机示例需要riscv64-unknown-elf-gcc/objcopy/nm。Python3可用时自动注册CLI/CSV端到端测试，否则跳过该项。没有交叉工具链时仅运行C++测试，不代表裸机程序集成已验收。
