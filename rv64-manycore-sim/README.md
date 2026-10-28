# rv64-manycore-sim

本版本新增每 Hart 独立的 MMIO 总线控制窗口（`0x90000000`）、可配置的目的端完整消息接收额度，以及命令行指令延迟参数。接口地址表、使用约定、时序限制和统计含义详见 [MMIO与背压说明](docs/mmio-and-backpressure.md)。

本次测试与18组实验原始结果见 [2026-10-28验收记录](docs/acceptance-2026-10-28.md)。

一个独立的众核 RISC-V 时序模拟器项目。目标是组合：

- `rv64-archsem`：RV64 与 V 扩展指令语义；
- `simbus_noc`：周期驱动的多通道 NoC 与拥塞/反压模型；
- 简单 Atomic 核心：每次只处理一条指令，每类指令使用预设周期；
- 共享内存控制器：请求排队、固定服务延迟和响应返回。

当前已经同时提供底层脚本核心和真实 `Rv64AtomicCore`。真实核心可以加载 raw binary，使用 `rv64-archsem` 解码并执行 RV64I/M 与第一阶段 RVV 整数指令，标量 Load/Store 通过 NoC 访问共享内存。

## 目录关系

在组合仓库中，三个模块并列放置，CMake自动发现相邻依赖：

```text
rv/
├── noc/                    # NoC 库，含转发背压与可选端到端额度
├── rv64-archsem/            # 修改后的 RV64+V 语义库快照
└── rv64-manycore-sim/       # 本项目
```

## 构建

```bash
cd rv64-manycore-sim
cmake -S . -B build
cmake --build build -j
(cd build && ctest --output-on-failure)
```

运行演示：

```bash
./build/manycore_demo
```

运行 RV64 raw binary：

```bash
./build/rv64_manycore_sim program.bin 4 0x80001000
```

第二个参数是核心数量，默认值为 2；第三个可选参数是 `tohost` MMIO 地址。所有核心使用同一份本地只读指令镜像，通过 `mhartid` 区分核心；数据 Load/Store 经 NoC 访问共享内存。

也可以使用具名参数配置实验，并把聚合统计追加到 CSV：

```bash
mkdir -p build/results
./build/rv64_manycore_sim build/examples/vector_stream_private.bin \
  --cores 4 --tohost 0x80001000 \
  --link-width 4 --route-latency 3 --router-buffer 2 \
  --memory-latency 8 --memory-queue 2 --max-ticks 2000000 \
  --workload vector_stream_private --csv build/results/private.csv
```

可配置项覆盖核心数、链路宽度、路由延迟、路由缓冲、内存延迟、内存队列和最大模拟周期。`--receive-slots`控制目的端消息额度，默认2；`--lat-alu`、`--lat-mul`、`--lat-div`、`--lat-valu`等设置指令类别周期，完整列表见接口说明。CSV schema版本2包含配置、完成状态、Hart等待、RVV事务、MMIO、NoC与内存服务统计；不能追加到旧版CSV。

## 拥塞实验负载

构建会生成三种持续 64 轮的 RVV 单位步长访存负载，每轮对 8 个 32 位元素执行“加载、加一、存回”：

- `vector_stream_private`：每个 Hart 使用独立地址区，观察纯流量扩展；
- `vector_stream_hotspot`：所有 Hart 访问同一地址区，形成共享热点；
- `vector_stream_gap`：私有地址区，但每轮插入计算间隔，降低请求注入率。

一条命令运行三种负载在 1/2/4/8 核下的基线扫描：

```bash
bash scripts/run_contention_sweep.sh build build/results/contention.csv
```

脚本不会覆盖已有 CSV；需要保留历史结果时可为每轮实验指定新文件名。

安装了 `riscv64-unknown-elf` 工具链时，构建过程会将裸机示例编译、链接并转换为raw binary，同时注册集成测试。工具链与Python3齐备时，当前共有10项CTest。也可以手动生成单个示例：

```bash
bash scripts/build_rv64_example.sh examples/tohost_smoke.S build/examples
./build/rv64_manycore_sim build/examples/tohost_smoke.bin 4 0x80001000
```

`tohost` 写操作仍然经过 NoC 和内存控制器。写入 `1` 表示测试通过；写入 `(测试编号 << 1) | 1` 表示失败。模拟器会在对应 Store 收到内存响应并退休后停止该 Hart。

## 当前时序规则

- 全局模拟采用单线程、确定性的逐周期调度。
- 每个核心普通及向量访存最多有一个未完成事务；MMIO适配器也只保留一个目标事务，并与普通访存串行使用端口。
- 核心先完成固定计算周期，再尝试向 NoC 注入请求。
- `can_send()`或`send()`失败时累计注入阻塞周期，包含源端背压和目的端无接收额度。
- 请求发出后，核心暂停，直到匹配的响应返回。
- 内存控制器使用有界队列和固定服务周期。
- NoC channel 0 传请求，channel 1 传响应。
- 同一 tick 发出的请求不会在同一 tick 返回。

## MMIO总线访问与结束协议

每Hart独立控制窗口位于 `0x90000000`，使用对齐的64位LD/SD访问地址、数据、宽度、命令、状态和结果寄存器。写COMMAND启动的目标读写才进入NoC；本地控制寄存器访问不经过NoC。适配器忙时可以执行本地计算和状态轮询，普通访存与Fence等待它完成。

```bash
./build/rv64_manycore_sim build/examples/mmio_bus_smoke.bin \
  --cores 4 --tohost 0x80001000 --receive-slots 1 \
  --memory-latency 100 --memory-queue 1
```

MMIO控制窗口与 `tohost` 不同：前者用于启动读写，后者用于裸机测试通过/失败上报。当前没有使用自定义CSR发起总线事务。完整地址表见 [接口说明](docs/mmio-and-backpressure.md)。

## 已支持的 RV64 范围

- RV64I 整数 ALU、分支和跳转
- RV64M 乘除法，并使用独立延迟
- CSR 基础读写，包括只读 `cycle`、`instret`、`mhartid`
- 标量整数 Load/Store
- Fence 在本 Hart 访存与 MMIO 适配器排空后提交
- `ecall`/`ebreak` 裸机停止
- 经 NoC 的基础 `tohost` pass/fail 退出协议
- 每 Hart 独立的 32×VLEN=256 位向量寄存器
- `vl`、`vtype`、`vlenb`、`vstart`、`vxrm`、`vxsat`、`vcsr` CSR
- `vsetvli`、`vsetivli`、`vsetvl` 基础配置流程
- LMUL=m1/m2/m4/m8 寄存器分组及首批整数向量加减、最值、逻辑和标量搬移指令
- EEW=SEW 时的 `vle8/16/32/64.v` 与 `vse8/16/32/64.v` 无掩码单位步长向量访存
- 向量 descriptor 按 8 字节边界拆分，经 NoC 串行发送并回填寄存器组

当前会明确拒绝压缩指令、AMO、浮点、特权返回、分数 LMUL、非单位步长/索引/分段 RVV 访存和尚未纳入白名单的向量指令，避免未建模指令静默产生错误结果。指令取指使用每核共享的本地只读 `ProgramImage`，不产生 NoC 流量。

服务器的 binutils 2.34 早于 RVV 1.0 汇编语法，`examples/vector_alu_smoke.S` 因此使用带指令名称注释的 `.word` 编码。生成的 raw binary 仍由真实交叉编译、链接和 objcopy 流程产生。

当前 `rv64-archsem` 的单位步长descriptor会给每个元素返回相同基础地址；这一问题在模拟器适配层通过恢复连续地址并按8字节边界重新合并处理，不需要进一步修改库。对应行为由 `vector_memory_smoke` 集成测试覆盖。

## 当前限制与后续方向

当前仅有一个内存控制器和一个活动服务请求，没有Cache、多Bank或一致性。Private与Hotspot共用同一个内存服务端点，地址不同本身不会增加或减少Bank冲突代价。Compute Gap只是插入计算间隔，没有DMA双缓冲。目的端额度检查即时生效，没有硬件额度返回延迟模型。

以下为可选扩展方向，尚未作为当前完成能力：

1. 基于 CSV 扫描核心数和 NoC/内存参数，定位吞吐拐点与主要阻塞来源。
2. 支持分数 LMUL、widen/narrow 和更多整数向量指令。
3. 扩展 strided/indexed/segment/whole-register RVV 访存。
4. 接入浮点、更多MMIO外设、AMO/LR/SC 和更多性能计数 CSR。
