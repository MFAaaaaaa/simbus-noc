# NoC与背压模型

`simbus_noc` 是可独立构建的C++17片上网络抽象模型，提供多通道消息传输、路由、分包/重组、有限转发缓冲、水位背压和网络压力测试。整个系统的介绍见 [仓库README](../README.md)。

## 文件组成

| 文件/目录 | 作用 |
|---|---|
| `include/simbus/businterface.h` | 发送、接收和周期推进公共接口 |
| `include/simbus/noc_config.h` | 链路、路由、缓冲、水位、接收额度配置及统计 |
| `include/simbus/symmulcha.h`、`src/symmulcha.cpp` | 多通道NoC、分包、路由推进、重组与背压 |
| `include/simbus/routetable.h`、`src/routetable.cpp` | 单环、双环、二维Mesh XY路由 |
| `tests/` | 路由、传输、拥塞、恢复及随机无丢包测试 |
| `examples/noc_benchmark.cpp` | 独立网络性能基准 |
| `scripts/`、`plots/` | 批量扫描、绘图脚本及历史示例图 |

## 构建与测试

以下命令均从仓库根目录执行：

```bash
cmake -S noc -B noc/build
cmake --build noc/build -j2
(cd noc/build && ctest --output-on-failure)
./noc/build/simbus_noc_tests
```

独立NoC注册1项CTest，内部包含15项测试，覆盖路由、同节点/跨节点传输、多通道、零长度消息、填充、拥塞背压和随机压力。

## 接口与背压

构造 `SymmetricMultiChannelBus` 时指定端口、端口对应节点、通道包宽、路由表和 `NocConfig`。调用方发送完整消息，模型按通道宽度分包；目的端按传输ID和包序号重组。

- `can_send(port, channel)`检查源端是否允许发送。
- `send(port, destination, channel, bytes)`尝试接纳整条消息；失败时必须保留并重试。
- `can_recv/recv`检查并取出完整消息。
- `apply_next_tick()`推进网络一个周期。

配置支持链路字节宽度、路由延迟、节点转发缓冲上限及高低水位。触及高水位时限制源端注入，下降到低水位后恢复；下游无法接纳转发包时保留上游包，背压逐级传播。

`receive_message_slots`限制每个目的端口/通道完整消息数，覆盖源端排队、在途、重组和等待应用接收。发送时预留，应用 `recv()` 后释放。有限模式下 `max_message_bytes`默认4096，超限报错。已预留消息的分包允许完成重组。

独立库默认slots=0，保留旧版无界目的端接收；众核系统默认slots=2。源端 `can_send()` 成功仍可能因为目的端无额度而导致 `send()` 失败。请求与响应可使用独立通道额度，但仍共享部分节点和链路。这是简化接纳控制，未建模额度返回延迟，详见 [系统背压说明](../rv64-manycore-sim/docs/mmio-and-backpressure.md)。

## 网络基准与绘图

```bash
./noc/build/noc_benchmark \
  --topology mesh --nodes 16 --mesh-x 4 --mesh-y 4 \
  --pattern hotspot --offered-load 0.8 --ticks 10000 \
  --buffer-limit 8 --high-watermark 6 --low-watermark 2
```

支持 `mesh`、`ring`、`double_ring` 拓扑及 `uniform`、`hotspot` 流量。`--congestion 1/0`切换转发缓冲拥塞控制；独立基准沿用库默认的无界目的端接收，不等同于众核有限额度实验。

基准输出CSV，包含注入/送达消息、平均包延迟、吞吐、阻塞、拥塞、在途消息和排空超时。节点累计阻塞周期可大于全局tick数。

```bash
bash noc/scripts/run_sweep.sh ./noc/build/noc_benchmark noc_sweep_new.csv
python3 noc/scripts/plot_sweep.py noc_sweep_new.csv --out-dir noc/plots-new
```

绘图需要Matplotlib。`noc/noc_sweep.csv`和`noc/plots/`为原有独立网络结果；新版众核数据见 [系统实验结果](../rv64-manycore-sim/docs/results/2026-10-28/)。

## 来源与许可证

模型源自 `nullrvsim` 总线/网络模型的设计与代码演化，采用 [MIT License](LICENSE)。再分发时请保留版权和许可证声明。
