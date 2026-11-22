# DX11 Renderer

C++ / DirectX 11 实时渲染器，包含实例化合批优化与分项 Profiling。

## 构建与运行

### 环境

- Visual Studio 2022（v143 工具集）
- Windows SDK 10.0.26100.0
- Release | x64
- 仅使用 Windows SDK、DirectXMath 和 WRL，无第三方依赖

### 构建

在 Visual Studio 2022 中打开 [DX11Renderer.sln](DX11Renderer.sln)，选择 **Release / x64** 后生成。

或在 Developer Command Prompt 中执行：

```bat
msbuild DX11Renderer.sln -p:Configuration=Release -p:Platform=x64 -m
```

输出文件：

```text
x64\Release\DX11Renderer.exe
```

### 运行

```bat
x64\Release\DX11Renderer.exe --n 10000 --instancing on
```

命令行参数：

| 参数 | 说明 |
|---|---|
| `--n <count>` | 立方体数量，默认 `200`，范围 1–10000 |
| `--instancing on` / `off` | 启用或关闭实例化渲染，默认 `off` |
| `--fixed-time <seconds>` | 验证用：冻结动画时间，便于像素级对比；默认不启用 |

`--instancing off` 走原来的 N 次 `DrawIndexed` 路径；`--instancing on` 走一次 `DrawIndexedInstanced`，`InstanceCount = N`。按 `Esc` 关闭窗口。

## 性能对比

数据来源：[baseline.md](docs/baseline.md) 与 [instancing-compare.md](docs/instancing-compare.md)。每组预热 3 秒，采样 5 次，表中取中位数。

| N | Instancing | DrawCalls | FrameTime (ms) | DrawSubmit (ms) |
|---:|:---:|---:|---:|---:|
| 1000 | off | 1000 | 0.24 | 0.03 |
| 1000 | on | 1 | 0.08 | 0.00 |
| 10000 | off | 10000 | 2.56 | 0.31 |
| 10000 | on | 1 | 0.59 | 0.00 |

**`FrameTime` 是 CPU 渲染提交耗时，不含 `Present` 与 vsync 等待。** `DrawSubmit` 是 `DrawIndexed` / `DrawIndexedInstanced` 调用本身的 QPC 累计耗时。

说明：

- 上表来自 [instancing-compare.md](docs/instancing-compare.md) 的同轮对比；绝对耗时会随桌面负载变化，DrawCalls 是确定值。
- 上表中的 `DrawSubmit: 0.00` 是两位小数显示。后续提高到三位小数并增加微秒后缀后，`--n 10000 --instancing on` 的精确值为 `0.0001 ms（0.100 us）`，见下方三段分解。
- [baseline.md](docs/baseline.md) 还记录了更早的非实例化扩展数据：N=200/1000 为 `0.04/0.17 ms`，N=2000/5000/10000 为 `0.39/0.86/1.71 ms`；这些是独立运行，不与上表混用。

### `--n 10000 --instancing on` 三段分解

以下为后续一次独立运行的 5 次采样中位数：

| 指标 | 中位数 |
|---|---:|
| FrameTime | 0.48 ms |
| DrawSubmit | 0.0001 ms（0.100 us） |
| WorldCalc | 0.380 ms |
| Upload | 0.041 ms |
| DrawCall | 0.0001 ms（0.100 us） |

说明：

- 该次运行的 `FrameTime` 与上表 `0.59 ms` 不同，属于运行间波动；两表数据来自不同采样轮次。
- `WorldCalc` 原始样本中有一个 `0.847 ms` 的调度离群值，表中使用中位数 `0.380 ms`。
- `DrawCall` 只计 `DrawIndexedInstanced` 调用本身。实测为 1 个 QPC tick（`0.0001 ms = 0.100 us`），低于预期的 `0.001–0.01 ms` 区间；三位小数毫秒会显示成 `0.000`，因此窗口标题额外显示微秒后缀，避免把非零值误显示为零。
- `WorldCalc`、`Upload`、`DrawCall` 三个分段只在 `--instancing on` 时显示；`off` 路径仍显示 DrawCalls、FrameTime、DrawSubmit。

## 正确性验证

使用 `--fixed-time 1.0` 将两种模式的动画时钟固定在同一个时刻，再分别以 `--n 200 --instancing off` 和 `--n 200 --instancing on` 启动并截图：

- [instancing-off-fixed.png](docs/instancing-off-fixed.png)
- [instancing-on-fixed.png](docs/instancing-on-fixed.png)

对比方法：截取窗口客户区，排除标题栏，比较 1296 × 729 像素的 RGB 值。结果：

```text
mean absolute RGB difference: 0.00012
maximum RGB difference:       1
pixels with difference > 2:   0
```

两种模式使用同一套网格、Shader、纹理、采样器和 `WorldMatrixForInstance` 世界矩阵计算；客户区平均绝对差 `0.00012`、最大 `1`，属于无可感知的舍入差异。标题栏文本不同（`DrawCalls: 200` 与 `DrawCalls: 1`），立方体数量、位置、旋转姿态和棋盘格纹理一致。

## 项目结构

| 文件 | 职责 |
|---|---|
| [main.cpp](main.cpp) | Win32 窗口、命令行解析、帧循环、标题栏 Profiling 显示 |
| [Renderer.h](Renderer.h) / [Renderer.cpp](Renderer.cpp) | D3D11 设备、SwapChain、深度缓冲、场景资源、实例化/非实例化绘制路径 |
| [Shader.h](Shader.h) / [Shader.cpp](Shader.cpp) | HLSL 编译、`VSMain`/`VSInstanced` 两个顶点入口、输入布局、像素着色器和采样器 |
| [Geometry.h](Geometry.h) / [Geometry.cpp](Geometry.cpp) | 立方体顶点/索引缓冲，绑定与 `DrawIndexed` 调用 |
| [Profiler.h](Profiler.h) / [Profiler.cpp](Profiler.cpp) | QPC 帧计时、60 帧平均、DrawCalls、DrawSubmit 及 WorldCalc/Upload/DrawCall 分段累计 |
| [DX11Renderer.vcxproj](DX11Renderer.vcxproj) / [DX11Renderer.sln](DX11Renderer.sln) | VS 2022 Release x64 工程与解决方案 |
| [docs/baseline.md](docs/baseline.md) | 非实例化扩展 N 的 FrameTime/DrawSubmit 基线及原始采样 |
| [docs/instancing-compare.md](docs/instancing-compare.md) | 实例化 on/off 对比、三段分解和像素级正确性验证 |
| [docs/benchmark.ps1](docs/benchmark.ps1) / [docs/capture.ps1](docs/capture.ps1) | 窗口标题采样和固定时刻截图辅助脚本 |

## 优化结论

**DrawCall 从 N 降至 1，剩余开销转移至世界矩阵计算与数据上传。**
