# simbus_noc

`simbus_noc` 是一个独立的 C++17 片上网络（NoC）抽象模拟模块，源自 `nullrvsim/src/bus` 中总线与网络模型的设计思路。项目提供周期驱动的多通道 NoC、路由表生成、拥塞控制、压力测试和性能基准工具。

## 主要功能

- `BusInterfaceV2`：基于端口和通道的发送/接收接口
- 单环、双环、二维 Mesh XY 路由表生成
- `SymmetricMultiChannelBus`：周期驱动的对称多通道 NoC
- 有界路由器缓冲区，以及基于高低水位线的拥塞检测与反压
- 多种拓扑和流量模式的性能基准
- 基于 CTest 的功能测试与随机压力测试
- CSV 批量实验和 Python 绘图工具

## 目录结构

```text
simbus_noc/
├── include/simbus/       # 公共头文件
├── src/                  # 路由表和 NoC 模型实现
├── tests/                # 功能、拥塞与压力测试
├── examples/             # NoC 性能基准程序
├── scripts/              # 批量实验和绘图脚本
├── plots/                # 示例实验图表
├── CMakeLists.txt
└── README.md
```

## 构建与测试

项目需要支持 C++17 的编译器和 CMake。

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

较旧版本的 CTest 可以进入构建目录运行：

```bash
cd build
ctest --output-on-failure
```

也可以直接执行测试程序：

```bash
./build/simbus_noc_tests
```

当前测试覆盖 15 个测试项，包括路由生成、基本传输、多通道隔离、零长度消息、拥塞反压、吞吐限制和随机无丢包压力测试。

## 最小使用示例

```cpp
#include "simbus/symmulcha.h"

using namespace simbus;

int main() {
    std::vector<BusNodeT> nodes{0, 1, 2, 3};
    BusRouteTable route;
    genroute_double_ring(nodes, route);

    NocConfig cfg;
    cfg.link_width_byte = 8;
    cfg.route_latency = 2;
    cfg.congestion.enabled = true;
    cfg.congestion.node_buffer_limit = 16;
    cfg.congestion.high_watermark = 12;
    cfg.congestion.low_watermark = 4;

    SymmetricMultiChannelBus bus({0, 1, 2, 3}, nodes, {4}, route, cfg);

    std::vector<uint8_t> payload{1, 2, 3, 4, 5};
    bus.send(0, 2, 0, payload);

    while (!bus.can_recv(2, 0)) {
        bus.apply_next_tick();
    }

    std::vector<uint8_t> received;
    bus.recv(2, 0, received);
}
```

## 拥塞控制模型

当前拥塞控制采用有界缓冲与反压机制：

1. 每个路由器维护有界的数据包缓冲计数。
2. 数据包进入中间路由器的转发流水线时占用一个缓冲槽。
3. 目的节点的接收和重组缓冲区保持无界，以符合原模拟器的简化假设，并避免多包消息产生死锁。
4. 如果接收转发数据包将超过缓冲上限，节点不会从上游链路输出或本地发送缓冲区取走该数据包。
5. 上游输出被占用后，反压会沿链路逐级传播。
6. 高、低水位线用于避免 `can_send()` 在临界点附近频繁振荡。

## 测试内容

- `test_routetable.cpp`：单环、双环、二维 Mesh XY 路由和非法节点检查
- `test_basic_transfer.cpp`：同节点、跨节点、多通道、零长度消息和填充移除
- `test_congestion.cpp`：反压与恢复、延迟、吞吐限制和源节点发送门控
- `test_stress.cpp`：普通与小缓冲区条件下的随机无丢包检查

成功时输出：

```text
[SUMMARY] 15 tests passed
```

## 性能基准

```bash
./build/noc_benchmark \
  --topology mesh --nodes 16 --mesh-x 4 --mesh-y 4 \
  --pattern hotspot --offered-load 0.8 --ticks 10000 \
  --buffer-limit 8 --high-watermark 6 --low-watermark 2
```

程序输出一行 CSV，包含注入与送达消息数、平均包延迟、吞吐量、阻塞/拥塞周期、源节点阻塞消息数、未送达与在途消息数，以及排空阶段是否超时。

### 拥塞控制对比

启用有界缓冲与反压：

```bash
./build/noc_benchmark --pattern hotspot --offered-load 0.8 \
  --congestion 1 --buffer-limit 8 --high-watermark 6 --low-watermark 2
```

关闭拥塞控制：

```bash
./build/noc_benchmark --pattern hotspot --offered-load 0.8 \
  --congestion 0 --buffer-limit 8 --high-watermark 6 --low-watermark 2
```

## 拓扑与流量模式

支持的拓扑：

- `mesh`：二维 Mesh XY 路由
- `ring`：单向环
- `double_ring`：双向环

支持的流量：

- `uniform`：每个源节点随机选择另一个目的节点
- `hotspot`：非热点节点以指定概率向热点节点发送消息

## 批量实验与绘图

```bash
./scripts/run_sweep.sh ./build/noc_benchmark noc_sweep.csv
python3 ./scripts/plot_sweep.py noc_sweep.csv --out-dir plots
```

绘图脚本会生成热点和均匀流量下的延迟、吞吐、阻塞周期、拥塞周期、送达比例和未送达消息等指标图。缺少 Matplotlib 时可运行：

```bash
python3 -m pip install matplotlib
```

## 许可证与来源

本项目采用 MIT License。NoC 模型源自 `nullrvsim` 总线/网络模型的设计与代码演化，仓库保留了原项目的版权声明和许可证。使用或再分发时请同时保留 `LICENSE` 文件。
