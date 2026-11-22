# DX11 Renderer CPU Profiling Baseline

- Date: 2026-11-18
- Configuration: Release / x64
- Resolution: 1280 x 720
- Compiler: MSVC 14.44.35207 (Visual Studio 2022, v143)
- Windows SDK: 10.0.26100.0
- Present: vsync enabled (`Present(1, 0)`)

## Measurement

`FrameTime` is the CPU-side render time measured with `QueryPerformanceCounter`
around the render submission work (clear, state setup, constant-buffer updates,
and all `DrawIndexed` calls). `Present` and its vsync wait are intentionally
outside this interval, because the project profiles CPU-side cost and a vsync
cap would hide the difference between different object counts.

`DrawSubmit` is an independent metric: before and after every individual
`DrawIndexed` call, a `QueryPerformanceCounter` sample is taken, and the deltas
are summed for the frame. It measures the CPU cost inside the draw calls
themselves (including the two QPC calls), and is therefore a subset of
`FrameTime`.

Each run was warmed up for 3 seconds, then the window title was sampled 5 times
at 500 ms intervals. The tables use the median sample. The window title format
is:

```text
DrawCalls: X | FrameTime: Y.YY ms | DrawSubmit: Z.ZZ ms
```

## Results

| Object count (`--n`) | DrawCalls | FrameTime (ms) |
|---:|---:|---:|
| 200 | 200 | 0.04 |
| 1000 | 1000 | 0.17 |

`DrawCalls` equals the object count because every cube is submitted with one
`DrawIndexed` call. All cubes share the same mesh, shader, texture, and sampler.

## Raw samples

`--n 200`:

```text
SAMPLE 1: DrawCalls: 200 | FrameTime: 0.04 ms
SAMPLE 2: DrawCalls: 200 | FrameTime: 0.04 ms
SAMPLE 3: DrawCalls: 200 | FrameTime: 0.04 ms
SAMPLE 4: DrawCalls: 200 | FrameTime: 0.04 ms
SAMPLE 5: DrawCalls: 200 | FrameTime: 0.04 ms
```

`--n 1000`:

```text
SAMPLE 1: DrawCalls: 1000 | FrameTime: 0.17 ms
SAMPLE 2: DrawCalls: 1000 | FrameTime: 0.19 ms
SAMPLE 3: DrawCalls: 1000 | FrameTime: 0.17 ms
SAMPLE 4: DrawCalls: 1000 | FrameTime: 0.18 ms
SAMPLE 5: DrawCalls: 1000 | FrameTime: 0.17 ms
```

## Larger N (DrawSubmit enabled)

The 200/1000 rows above were recorded before `DrawSubmit` instrumentation and
are retained unchanged. The new runs use the same warm-up and sampling method:

| Object count (`--n`) | DrawCalls | FrameTime (ms) | DrawSubmit (ms) |
|---:|---:|---:|---:|
| 2000 | 2000 | 0.39 | 0.05 |
| 5000 | 5000 | 0.86 | 0.14 |
| 10000 | 10000 | 1.71 | 0.26 |

`--n 2000` raw samples:

```text
SAMPLE 1: DrawCalls: 2000 | FrameTime: 0.38 ms | DrawSubmit: 0.04 ms
SAMPLE 2: DrawCalls: 2000 | FrameTime: 0.39 ms | DrawSubmit: 0.05 ms
SAMPLE 3: DrawCalls: 2000 | FrameTime: 0.39 ms | DrawSubmit: 0.06 ms
SAMPLE 4: DrawCalls: 2000 | FrameTime: 0.41 ms | DrawSubmit: 0.10 ms
SAMPLE 5: DrawCalls: 2000 | FrameTime: 0.38 ms | DrawSubmit: 0.04 ms
```

`--n 5000` raw samples:

```text
SAMPLE 1: DrawCalls: 5000 | FrameTime: 0.85 ms | DrawSubmit: 0.15 ms
SAMPLE 2: DrawCalls: 5000 | FrameTime: 0.83 ms | DrawSubmit: 0.13 ms
SAMPLE 3: DrawCalls: 5000 | FrameTime: 0.86 ms | DrawSubmit: 0.15 ms
SAMPLE 4: DrawCalls: 5000 | FrameTime: 0.86 ms | DrawSubmit: 0.14 ms
SAMPLE 5: DrawCalls: 5000 | FrameTime: 0.88 ms | DrawSubmit: 0.12 ms
```

`--n 10000` raw samples:

```text
SAMPLE 1: DrawCalls: 10000 | FrameTime: 1.77 ms | DrawSubmit: 0.23 ms
SAMPLE 2: DrawCalls: 10000 | FrameTime: 1.65 ms | DrawSubmit: 0.31 ms
SAMPLE 3: DrawCalls: 10000 | FrameTime: 1.71 ms | DrawSubmit: 0.26 ms
SAMPLE 4: DrawCalls: 10000 | FrameTime: 1.71 ms | DrawSubmit: 0.19 ms
SAMPLE 5: DrawCalls: 10000 | FrameTime: 1.73 ms | DrawSubmit: 0.32 ms
```

## Notes

- The original single-cube path is preserved: `--n 1` creates one instance at
  the origin with the original rotation speeds, and its smoke-test title was
  `DrawCalls: 1 | FrameTime: 0.00 ms | DrawSubmit: 0.00 ms` (CPU time below
  the 2-decimal display).
- `DrawSubmit` measures CPU time around `DrawIndexed` only. It is not GPU draw
  time and does not include vertex/index/topology binding.
- With vsync enabled, the wall-clock frame interval was approximately 6.06 ms;
  that includes the `Present` wait and is not the CPU render time reported
  above.
- The benchmark helper is [benchmark.ps1](<C:/Users/fengsu/projects/dx11-renderer/docs/benchmark.ps1>).
