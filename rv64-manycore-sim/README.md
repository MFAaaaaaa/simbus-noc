# rv64-manycore-sim

一个独立的众核 RISC-V 时序模拟器项目。目标是组合：

- `rv64-archsem`：RV64 与 V 扩展指令语义；
- `simbus_noc`：周期驱动的多通道 NoC 与拥塞/反压模型；
- 简单 Atomic 核心：每次只处理一条指令，每类指令使用预设周期；
- 共享内存控制器：请求排队、固定服务延迟和响应返回。

当前已经同时提供底层脚本核心和真实 `Rv64AtomicCore`。真实核心可以加载 raw binary，使用 `rv64-archsem` 解码并执行 RV64I/M 与第一阶段 RVV 整数指令，标量 Load/Store 通过 NoC 访问共享内存。

## 目录关系

本项目独立于 `bus-test`：

```text
rv/
├── bus-test/simbus_noc/     # NoC 库，不由本项目修改
├── rv64-archsem/            # RV64+V 语义库，不由本项目修改
└── rv64-manycore-sim/       # 本项目
```

## 构建

```bash
cd /home/aii-works/rv/rv64-manycore-sim
cmake -S . -B build \
  -DSIMBUS_NOC_ROOT=/home/aii-works/rv/bus-test/simbus_noc \
  -DRV64_ARCHSEM_ROOT=/home/aii-works/rv/rv64-archsem
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
./build/rv64_manycore_sim build/examples/vector_stream_private.bin \
  --cores 4 --tohost 0x80001000 \
  --link-width 4 --route-latency 3 --router-buffer 2 \
  --memory-latency 8 --memory-queue 2 --max-ticks 2000000 \
  --workload vector_stream_private --csv build/results/private.csv
```

可配置项覆盖核心数、链路宽度、路由延迟、路由缓冲、内存延迟、内存队列和最大模拟周期。CSV 包含完成状态、总周期、各类核心等待周期、RVV 指令/事务数、NoC 包延迟与阻塞周期，以及内存队列吞吐统计。

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

服务器安装了 `riscv64-unknown-elf` 工具链时，构建过程会把 `examples/tohost_smoke.S` 编译、链接并转换为 raw binary，同时注册一项 4 核 CTest。也可以手动生成：

```bash
bash scripts/build_rv64_example.sh examples/tohost_smoke.S build/examples
./build/rv64_manycore_sim build/examples/tohost_smoke.bin 4 0x80001000
```

`tohost` 写操作仍然经过 NoC 和内存控制器。写入 `1` 表示测试通过；写入 `(测试编号 << 1) | 1` 表示失败。模拟器会在对应 Store 收到内存响应并退休后停止该 Hart。

## 当前时序规则

- 全局模拟采用单线程、确定性的逐周期调度。
- 每个核心最多有一个未完成内存事务。
- 核心先完成固定计算周期，再尝试向 NoC 注入请求。
- `can_send()` 为 false 时累计注入阻塞周期。
- 请求发出后，核心暂停，直到匹配的响应返回。
- 内存控制器使用有界队列和固定服务周期。
- NoC channel 0 传请求，channel 1 传响应。
- 同一 tick 发出的请求不会在同一 tick 返回。

## 已支持的 RV64 范围

- RV64I 整数 ALU、分支和跳转
- RV64M 乘除法，并使用独立延迟
- CSR 基础读写，包括只读 `cycle`、`instret`、`mhartid`
- 标量整数 Load/Store
- Fence 空操作提交
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

当前 `rv64-archsem` 的单位步长 descriptor 会给每个元素返回相同基础地址；本项目不修改该库，而是在适配层使用 descriptor 的寄存器偏移恢复连续地址，并按 8 字节边界重新合并。对应行为由 `vector_memory_smoke` 集成测试覆盖。

## 下一阶段

1. 基于 CSV 扫描核心数和 NoC/内存参数，定位吞吐拐点与主要阻塞来源。
2. 支持分数 LMUL、widen/narrow 和更多整数向量指令。
3. 扩展 strided/indexed/segment/whole-register RVV 访存。
4. 接入浮点、通用 MMIO、AMO/LR/SC 和更多性能计数 CSR。
