# simbus-noc：众核 RV64+V 总线拥塞模拟系统

本项目将支持部分 RV64I/M 与 V 扩展指令的 Atomic Hart、带背压的 NoC 和共享内存集成为一个逐周期模拟器，用于研究多核访存竞争、请求排队、网络阻塞和核心停顿。

核心按指令类别使用预设执行周期。多个 Hart 在模拟时间上并发运行；模拟器使用单线程、确定性的全局 tick 推进状态。项目定位为可运行裸机程序的系统级拥塞实验基线。

## 代码组成

```text
simbus-noc/
├── noc/                  # NoC、路由、背压、独立网络测试与基准
├── rv64-archsem/         # 修改后的 RV64/RVV 指令解码与语义库
├── rv64-manycore-sim/    # Atomic Hart、内存、MMIO与系统调度
└── README.md
```

| 模块 | 职责 |
|---|---|
| [noc](noc/README.md) | 分包/重组、路由、有限转发缓冲、水位背压与可选目的端接收额度；可独立构建 |
| [rv64-archsem](rv64-archsem/README.md) | 指令解码、运算语义和访存描述生成；收录修改后的源码快照 |
| [rv64-manycore-sim](rv64-manycore-sim/README.md) | 组合核心、NoC和内存，加载裸机程序，记录时序与阻塞；依赖前两个目录 |

## 系统架构与能力

```text
多个 RV64+V Atomic Hart
    │ 普通标量/向量访存，或MMIO适配器发起的目标事务
    ↕
带背压的 NoC（通道0：请求；通道1：响应）
    ↕
内存控制器（有限请求队列、固定服务周期）
    ↕
共享内存（稀疏字节存储）

统一逐周期调度器推进上述模块并收集统计
```

- 可配置Hart数量，已验证1/2/4/8 Hart；每Hart有独立PC、整数/向量寄存器、CSR和 `mhartid`。
- 支持RV64整数计算、分支、标量访存、乘除法和基础CSR；所有Hart使用同一份本地只读程序镜像，取指不产生NoC流量。
- RVV子集：VLEN=256、SEW=8/16/32/64、LMUL=m1/m2/m4/m8；向量配置、基础整数加减/最值/逻辑/搬移，以及EEW=SEW的无掩码单位步长访存。
- 指令类别延迟可配置：整数ALU默认1周期、乘法3周期、除法18周期、向量整数ALU4周期；取指、请求准备和动态等待另计。
- 每Hart在 `0x90000000` 有独立MMIO控制窗口，可设置地址、数据和大小、启动总线读写并查询结果；未采用自定义CSR发起总线请求。
- NoC库支持单环、双环、二维Mesh XY路由；当前众核系统使用双环。转发缓冲采用水位背压，众核默认每个目的端口/通道预留2条完整消息的接收额度。
- `tohost` 特殊地址协议负责裸机程序通过/失败上报，与MMIO总线控制窗口是不同接口。
- 输出每Hart统计及聚合CSV，覆盖执行、注入阻塞、响应等待、MMIO、NoC和内存队列指标。

完整地址表、延迟参数与统计口径见 [MMIO与背压说明](rv64-manycore-sim/docs/mmio-and-backpressure.md)。

## 快速开始

以下命令在Linux环境、仓库根目录执行。独立NoC需要C++17；完整系统需要C++20编译器、CMake和bash。裸机示例需要 `riscv64-unknown-elf-gcc`、`riscv64-unknown-elf-objcopy` 和 `riscv64-unknown-elf-nm`。Python3用于可选的CLI/CSV端到端测试。

### 构建与测试

```bash
cmake -S rv64-manycore-sim -B rv64-manycore-sim/build
cmake --build rv64-manycore-sim/build -j2
(cd rv64-manycore-sim/build && ctest --output-on-failure)
```

CMake自动定位相邻的 `noc/` 和 `rv64-archsem/`。有交叉工具链时会将 `.S` 裸机示例编译、链接并转换为raw binary；没有工具链时跳过程序集成测试，不能将其视为完整验收。

### 日常运行：一条短命令

```bash
cd rv64-manycore-sim
python3 run.py private
```

**核心数、NoC、内存和指令周期统一在 [run.ini](rv64-manycore-sim/run.ini) 中修改，无需重新编译。** 默认4核；`[common]`是公共参数，各场景段可覆盖。统一入口需要Python3.7或更高版本。

在同一目录也可以运行：

```bash
python3 run.py hotspot     # 共享热点
python3 run.py compute     # 计算间隔+向量访存
python3 run.py mmio        # MMIO总线读写
python3 run.py all         # 四个场景各运行一次
python3 run.py sweep       # 三类RVV负载扫描1/2/4/8核，共12组
python3 run.py private --cores 8  # 临时覆盖核心数
```

终端每组显示PASS/FAIL和总周期；完整日志、CSV和实际配置快照自动写入独立的 `build/results/时间戳-随机后缀/`，不会覆盖历史结果。`--dry-run`仅预览最终命令。原始命令行仍可用，详见 [运行说明](rv64-manycore-sim/README.md)。

### 批量复现实验

以下18组完整验收脚本从仓库根目录执行（如果仍在模拟器目录，先执行 `cd ..`）：

```bash
bash rv64-manycore-sim/scripts/run_system_acceptance.sh \
  rv64-manycore-sim/build rv64-manycore-sim/build/results/acceptance
```

脚本运行三类RVV负载的1/2/4/8核扫描、四组MMIO读写和两组接收额度对照，共18组。已有结果目录不会被覆盖。CSV使用schema版本2，不能与旧版结果混合追加。独立网络构建、基准与绘图见 [NoC使用说明](noc/README.md)。

## 验证结果

2026-10-28在服务器完成的验证：

| 验证 | 结果 |
|---|---|
| 独立NoC | 1项CTest，内部15项测试全部通过 |
| 众核系统 | 10/10项CTest通过，包含MMIO、额度、恢复、延迟与裸机测试 |
| 系统实验 | 18/18组成功，原始CSV随仓库保存 |

统一INI入口另有 `runner_ini_acceptance` 验收，覆盖参数继承、临时覆盖、短命令运行和历史结果保留；工具链与Python3齐备时，当前众核CTest总数为11。

测试条件和逐规模结果见 [验收记录](rv64-manycore-sim/docs/acceptance-2026-10-28.md)，原始数据见 [实验CSV](rv64-manycore-sim/docs/results/2026-10-28/)。这些周期值来自简化模型，不是实际芯片性能测量。

## 负载与模型限制

三类RVV负载均执行64轮“加载8个32位元素—每元素加1—存回”：Private使用每Hart私有地址，Hotspot使用共享地址，Compute Gap在每轮之间插入标量计算间隔。Compute Gap是计算与访存交替，不是DMA双缓冲或计算访存重叠。

- 单内存控制器、单活动服务请求，没有Cache、多Bank或一致性；Private与Hotspot没有额外的地址冲突时序差异。
- 每Hart普通/向量访存最多一个未完成事务，向量子事务串行发送。MMIO适配器也只保留一个目标事务；它忙时可继续本地计算和状态轮询，普通访存等待排空。
- RVV是受限子集，没有分数LMUL、浮点、索引/分段访存，也没有AMO/LR/SC、虚拟内存和完整操作系统环境。
- 目的端额度属于简化接纳控制，额度检查即时可见，没有硬件额度返回延迟；测试通过不代表对任意拓扑的无死锁证明。
- Hart等待与NoC节点阻塞是累计量，可能大于全局模拟周期；不能将所有指标直接相加。

## 来源与许可证

NoC源自 `nullrvsim` 总线/网络模型的代码演化，保留 [noc/LICENSE](noc/LICENSE) 中的MIT声明。`rv64-archsem`来自 [meng-cz/rv64-archsem](https://github.com/meng-cz/rv64-archsem)，基于上游 `b26e726` 工作副本并包含本地修改，许可证见 [rv64-archsem/LICENSE](rv64-archsem/LICENSE)。第三方源码保留各自版权与许可声明。
