# Instancing Comparison

- Date: 2026-11-18
- Configuration: Release / x64
- Resolution: 1280 x 720
- Compiler: MSVC 14.44.35207 (Visual Studio 2022, v143)
- Windows SDK: 10.0.26100.0
- Present: vsync enabled (`Present(1, 0)`)
- `FrameTime`: CPU render submission time, excluding `Present`/vsync wait
- `DrawSubmit`: sum of `QueryPerformanceCounter` intervals around each
  `DrawIndexed` / `DrawIndexedInstanced` call

## Result

| Object count (`--n`) | Instancing | DrawCalls | FrameTime (ms) | DrawSubmit (ms) |
|---:|:---:|---:|---:|---:|
| 1000 | off | 1000 | 0.24 | 0.03 |
| 1000 | on | 1 | 0.08 | 0.00 |
| 10000 | off | 10000 | 2.56 | 0.31 |
| 10000 | on | 1 | 0.59 | 0.00 |

**Conclusion: with instancing enabled, the DrawCall count drops from N to 1**
(1000 -> 1 and 10000 -> 1). The off path remains the original N-call path.

`DrawSubmit` for the instanced path is below the two-decimal title display
precision in this earlier table; the timing-breakdown section below reports the
same instanced path at three decimals plus a microsecond suffix.

The values are 5-sample medians from the final Release x64 build. Absolute
timings vary with desktop load, but the DrawCall counts are deterministic:
`off` reports N and `on` reports 1.

## Raw samples

`--n 1000 --instancing off`:

```text
SAMPLE 1: DrawCalls: 1000 | FrameTime: 0.27 ms | DrawSubmit: 0.04 ms
SAMPLE 2: DrawCalls: 1000 | FrameTime: 0.23 ms | DrawSubmit: 0.03 ms
SAMPLE 3: DrawCalls: 1000 | FrameTime: 0.24 ms | DrawSubmit: 0.03 ms
SAMPLE 4: DrawCalls: 1000 | FrameTime: 0.25 ms | DrawSubmit: 0.02 ms
SAMPLE 5: DrawCalls: 1000 | FrameTime: 0.23 ms | DrawSubmit: 0.04 ms
```

`--n 1000 --instancing on`:

```text
SAMPLE 1: DrawCalls: 1 | FrameTime: 0.13 ms | DrawSubmit: 0.00 ms
SAMPLE 2: DrawCalls: 1 | FrameTime: 0.08 ms | DrawSubmit: 0.00 ms
SAMPLE 3: DrawCalls: 1 | FrameTime: 0.08 ms | DrawSubmit: 0.00 ms
SAMPLE 4: DrawCalls: 1 | FrameTime: 0.10 ms | DrawSubmit: 0.00 ms
SAMPLE 5: DrawCalls: 1 | FrameTime: 0.06 ms | DrawSubmit: 0.00 ms
```

`--n 10000 --instancing off`:

```text
SAMPLE 1: DrawCalls: 10000 | FrameTime: 2.56 ms | DrawSubmit: 0.31 ms
SAMPLE 2: DrawCalls: 10000 | FrameTime: 2.56 ms | DrawSubmit: 0.31 ms
SAMPLE 3: DrawCalls: 10000 | FrameTime: 2.56 ms | DrawSubmit: 0.31 ms
SAMPLE 4: DrawCalls: 10000 | FrameTime: 2.56 ms | DrawSubmit: 0.31 ms
SAMPLE 5: DrawCalls: 10000 | FrameTime: 2.56 ms | DrawSubmit: 0.31 ms
```

`--n 10000 --instancing on`:

```text
SAMPLE 1: DrawCalls: 1 | FrameTime: 0.75 ms | DrawSubmit: 0.00 ms
SAMPLE 2: DrawCalls: 1 | FrameTime: 0.64 ms | DrawSubmit: 0.00 ms
SAMPLE 3: DrawCalls: 1 | FrameTime: 0.54 ms | DrawSubmit: 0.00 ms
SAMPLE 4: DrawCalls: 1 | FrameTime: 0.59 ms | DrawSubmit: 0.00 ms
SAMPLE 5: DrawCalls: 1 | FrameTime: 0.50 ms | DrawSubmit: 0.00 ms
```

## Visual verification

Both modes use the same per-instance world-matrix helper, mesh, shader,
texture, sampler, and pixel shader. The only difference is how the world
matrices reach the vertex shader:

- `off`: per-object constant-buffer update, then N `DrawIndexed` calls.
- `on`: one DYNAMIC instance buffer update, then one
  `DrawIndexedInstanced(..., InstanceCount = N)` call.

For an exact visual comparison, the validation-only `--fixed-time 1.0` option
was used to freeze the animation clock at the same time in both runs. Fixed-time
screenshots at `--n 200`:

- [instancing-off-fixed.png](<C:/Users/fengsu/projects/dx11-renderer/docs/instancing-off-fixed.png>)
- [instancing-on-fixed.png](<C:/Users/fengsu/projects/dx11-renderer/docs/instancing-on-fixed.png>)

Client-area comparison (title bar excluded, 1296 x 729 pixels):

```text
mean absolute RGB difference: 0.00012
maximum RGB difference:       1
pixels with difference > 2:   0
```

This confirms the cube count, positions, rotations, and checkerboard texture
match between the two paths. The only visible difference is the title bar
(`DrawCalls: 200` vs `DrawCalls: 1`).

## Timing breakdown (n=10000, instancing on)

Five samples, 3-second warm-up, 500 ms sampling interval. Medians:

| Metric | Median |
|---|---:|
| FrameTime | 0.48 ms |
| DrawSubmit | 0.0001 ms (0.100 us) |
| WorldCalc | 0.380 ms |
| Upload | 0.041 ms |
| DrawCall | 0.0001 ms (0.100 us) |

The title shows `DrawSubmit: 0.000 ms (0.100 us)` and
`DrawCall: 0.000 ms (0.100 us)`: the required three-decimal millisecond field
rounds 0.0001 ms to 0.000, so a microsecond suffix is shown to keep the real,
nonzero value visible. The exact `DrawIndexedInstanced` call measures one QPC
tick (0.1 us), not the predicted 0.001–0.01 ms range. This is a real measurement
on this host, not a zeroed placeholder.

Raw samples:

```text
SAMPLE 1: DrawCalls: 1 | FrameTime: 0.48 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.847 ms | Upload: 0.033 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 2: DrawCalls: 1 | FrameTime: 0.45 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.380 ms | Upload: 0.041 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 3: DrawCalls: 1 | FrameTime: 0.50 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.364 ms | Upload: 0.048 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 4: DrawCalls: 1 | FrameTime: 0.49 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.445 ms | Upload: 0.031 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 5: DrawCalls: 1 | FrameTime: 0.45 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.357 ms | Upload: 0.043 ms | DrawCall: 0.000 ms (0.100 us)
```

The 0.847 ms WorldCalc sample is a scheduling outlier; the table uses the
5-sample median.

## Implementation notes

- Per-instance matrix layout: `D3D11_INPUT_PER_INSTANCE_DATA`, input slot 1,
  `InstanceDataStepRate = 1`, four `INSTANCE_WORLD` float4 elements.
- The instance buffer uses `D3D11_USAGE_DYNAMIC` and
  `D3D11_CPU_ACCESS_WRITE`; it is updated once per frame with
  `D3D11_MAP_WRITE_DISCARD`.
- `ByteWidth` is `sizeof(DirectX::XMFLOAT4X4) * N`, rounded up to a 16-byte
  boundary.
- `--instancing on/off` defaults to `off`, preserving the original
  N-`DrawIndexed` path.
- `--fixed-time` is a validation-only option; it does not affect the default
  animation path.
- On the instanced path, `WorldCalc`, `Upload`, and `DrawCall` are separate
  per-frame accumulators. `DrawCall` times only the `DrawIndexedInstanced`
  call; state binding is outside it.
- `DrawSubmit` and `DrawCall` are the same single-call interval on the
  instanced path. On the non-instanced path, `DrawSubmit` is the sum of the N
  per-call intervals.
- The title uses `%.3f` ms for all timing fields and appends a `%.3f us`
  value for `DrawSubmit` and `DrawCall` so sub-0.001 ms measurements remain
  visible without claiming millisecond precision that the display cannot
  represent.

## PBR per-instance material (current build)

The material parameters were moved from `SceneConstants` into the
per-instance buffer. `InstanceData` is 96 bytes:

```text
world      offset 0    64 bytes
albedo     offset 64   16 bytes
metallic   offset 80    4 bytes
roughness  offset 84    4 bytes
padding    offset 88    8 bytes
```

The instanced input layout reads `INSTANCE_ALBEDO` at offset 64 and
`INSTANCE_MATERIAL` (`R32G32_FLOAT`, metallic + roughness) at offset 80. The
pixel shader receives albedo/metallic/roughness from the vertex shader and no
longer reads a material constant buffer. The off path keeps the 5x5 reference
scene and uses a separate per-object material constant buffer for its
`DrawIndexed` path.

`--instancing off` keeps the 5x5 reference scene regardless of `--n`;
`--instancing on` renders `--n` random spheres (default 10000) with random
albedo, metallic, and roughness.

### PBR results

| Scene | N | Instancing | DrawCalls | FrameTime (ms) | DrawSubmit (ms) | WorldCalc (ms) | Upload (ms) | DrawCall (ms) |
|---|---:|:---:|---:|---:|---:|---:|---:|---:|
| PBR 5x5 reference | 25 | off | 25 | 0.01 | 0.001 | - | - | - |
| PBR random spheres | 1000 | on | 1 | 0.03 | 0.0001 | 0.018 | 0.007 | 0.0001 |
| PBR random spheres | 10000 | on | 1 | 0.16 | 0.0002 | 0.092 | 0.063 | 0.0002 |

`--n 10000 --instancing off` was also checked and still reports
`DrawCalls: 25`, confirming that the off path is the fixed 5x5 visual
baseline.

### Comparison with the earlier non-PBR runs

| Version | N | Instancing | DrawCalls | FrameTime (ms) | DrawSubmit (ms) |
|---|---:|---:|---:|---:|---:|
| Non-PBR cubes | 10000 | off | 10000 | 2.56 | 0.31 |
| Non-PBR cubes | 10000 | on | 1 | 0.59 | 0.00 |
| PBR spheres | 10000 | on | 1 | 0.16 | 0.0002 |
| PBR 5x5 reference | 25 | off | 25 | 0.01 | 0.001 |

The absolute timings are not directly apples-to-apples: the old test used
12-triangle cubes and a single material, while the new test uses 64x32 UV
spheres (4096 triangles each), per-instance materials, and a different
off-path scene size. The stable comparison is the DrawCall count: the
instanced PBR path drops from N calls to 1. The remaining CPU cost is now
WorldCalc + Upload rather than per-object draw submission.

### PBR raw samples

`--n 25 --instancing off`:

```text
SAMPLE 1: DrawCalls: 25 | FrameTime: 0.01 ms | DrawSubmit: 0.001 ms (0.500 us)
SAMPLE 2: DrawCalls: 25 | FrameTime: 0.01 ms | DrawSubmit: 0.001 ms (0.500 us)
SAMPLE 3: DrawCalls: 25 | FrameTime: 0.01 ms | DrawSubmit: 0.001 ms (0.600 us)
SAMPLE 4: DrawCalls: 25 | FrameTime: 0.01 ms | DrawSubmit: 0.001 ms (0.600 us)
SAMPLE 5: DrawCalls: 25 | FrameTime: 0.01 ms | DrawSubmit: 0.000 ms (0.400 us)
```

`--n 1000 --instancing on`:

```text
SAMPLE 1: DrawCalls: 1 | FrameTime: 0.03 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.045 ms | Upload: 0.006 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 2: DrawCalls: 1 | FrameTime: 0.04 ms | DrawSubmit: 0.000 ms (0.200 us) | WorldCalc: 0.008 ms | Upload: 0.006 ms | DrawCall: 0.000 ms (0.200 us)
SAMPLE 3: DrawCalls: 1 | FrameTime: 0.03 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.004 ms | Upload: 0.007 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 4: DrawCalls: 1 | FrameTime: 0.04 ms | DrawSubmit: 0.000 ms (0.100 us) | WorldCalc: 0.036 ms | Upload: 0.013 ms | DrawCall: 0.000 ms (0.100 us)
SAMPLE 5: DrawCalls: 1 | FrameTime: 0.03 ms | DrawSubmit: 0.000 ms (0.200 us) | WorldCalc: 0.018 ms | Upload: 0.014 ms | DrawCall: 0.000 ms (0.200 us)
```

`--n 10000 --instancing on`:

```text
SAMPLE 1: DrawCalls: 1 | FrameTime: 0.17 ms | DrawSubmit: 0.000 ms (0.200 us) | WorldCalc: 0.077 ms | Upload: 0.064 ms | DrawCall: 0.000 ms (0.200 us)
SAMPLE 2: DrawCalls: 1 | FrameTime: 0.16 ms | DrawSubmit: 0.000 ms (0.200 us) | WorldCalc: 0.088 ms | Upload: 0.057 ms | DrawCall: 0.000 ms (0.200 us)
SAMPLE 3: DrawCalls: 1 | FrameTime: 0.16 ms | DrawSubmit: 0.001 ms (0.500 us) | WorldCalc: 0.109 ms | Upload: 0.054 ms | DrawCall: 0.001 ms (0.500 us)
SAMPLE 4: DrawCalls: 1 | FrameTime: 0.15 ms | DrawSubmit: 0.000 ms (0.200 us) | WorldCalc: 0.136 ms | Upload: 0.063 ms | DrawCall: 0.000 ms (0.200 us)
SAMPLE 5: DrawCalls: 1 | FrameTime: 0.16 ms | DrawSubmit: 0.000 ms (0.300 us) | WorldCalc: 0.092 ms | Upload: 0.071 ms | DrawCall: 0.000 ms (0.300 us)
```

Screenshots:

- [PBR 5x5 reference](<C:/Users/fengsu/projects/dx11-renderer/docs/pbr-spheres-instancing-off.png>)
- [PBR 10000 instanced spheres](<C:/Users/fengsu/projects/dx11-renderer/docs/pbr-spheres-instancing-on.png>)
