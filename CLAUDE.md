# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

Requires Visual Studio 2022+ and Windows 10/11. Solution: `Bengals/Bengals.sln` (MSVC v143, C++20, x64/Win32).

```bash
# 1. Restore NuGet packages (Agility SDK + DirectXTex)
nuget restore ./Bengals/Bengals.sln

# 2. Copy D3D12 runtime binaries (required before first run)
# From: packages/Microsoft.Direct3D.D3D12.*/build/native/bin/{x64,arm64,win32}
# To:   Bengals/D3D12/

# 3. Build
msbuild ./Bengals/Bengals.sln /t:Build /p:Configuration=Debug /p:Platform=x64

# 4. Run from repo root (so shaders and D3D12/ runtime resolve correctly)
./Bengals/x64/Debug/Bengals_x64_debug.exe
```

Alternative IDE build: `devenv ./Bengals/Bengals.sln /Build "x64|Debug" /Project Bengals`

Shader compilation: `fxc /T ps_5_1 /Fo DefaultShader.cso DefaultShader.hlsl` (or via project custom build step). Commit both `.hlsl` source and `.cso` output.

No automated test suite. Smoke-test by running the debug build, resizing the window, toggling Alt+Enter, and checking D3D12 debug output for warnings or live-object reports. Validate GPU resource changes with PIX or `d3d12sdklayers.dll` logging before merging. Capture screenshots or GIFs when visual output changes.

Build artifacts land in `Bengals/x64/<Config>/`; never commit `.exe`, `.pdb`, or generated `.cso` under that tree.

## Coding Conventions

- **Classes**: `C` prefix + PascalCase (`CRenderer`, `CResourceManager`)
- **Members**: `m_` + camelCase; `m_b` for bool; `m_p` for raw pointer; `m_pp` for double pointer; `unique_ptr`/`ComPtr` 등 스마트 포인터는 `m_p` 접두사 없이 `m_` + camelCase
- **Globals**: `g_` prefix, same suffixes as members (`g_pMeshObject`, `g_bEnable`)
- **Locals/params**: camelCase; `b` prefix for bool; `p`/`pp` prefix for pointers
- **Functions**: PascalCase. Lifecycle pattern: `OnInit`, `LoadPipeline`, `LoadAssets`, `OnUpdate`, `OnRender`, `OnDestroy`, `PopulateCommandList`, `WaitForPreviousFrame`
- **Structs**: PascalCase type name; public fields PascalCase; bool fields `b` prefix
- **Constants**: PascalCase (`FrameCount`); macros ALL_CAPS (prefer `constexpr`/`const`)
- **Files/folders**: PascalCase. D3D12 backend boundaries use `D3D12*` or `Dx12*` prefix
- **Enums**: PascalCase name and values
- Tabs for indentation, braces on new lines, switch cases indented one tab
- Include `pch.h` first; favor `ComPtr`/`std::unique_ptr` over raw pointers
- Keep shader files (`Renderer/Shaders/*.hlsl` + `.cso`) in sync with their filenames
- Commit messages: subsystem-prefixed imperative (`renderer: fix swap chain resize`), <=72 chars, short body describing intent and manual tests
- PRs must link issues/tasks, summarize the change, list validation steps (build/run commands, GPUs tested), mention assets or SDK files touched, and attach visuals for rendering tweaks

## Architecture

### Renderer Pipeline (`Bengals/Renderer/`)

**`CD3D12Renderer`** (`D3D12Renderer.h/cpp`) — Singleton owning the D3D12 device, swap chain (2 buffers), command queue/list/allocator, fence, RTV/DSV descriptor heaps, camera matrices. Public API exposes `BeginRender()`/`EndRender()`/`Present()` frame lifecycle and mesh/texture creation via opaque `void*` handles.

**`CBasicMeshObject`** (`BasicMeshObject/`) — Per-mesh vertex/index buffers. Root signature and PSO are **static** (shared across all instances) with reference counting for cleanup.

**`CD3D12ResourceManager`** (`ResourceManager/`) — Owns a **dedicated** command queue/allocator/list for GPU uploads (parallel to the main render queue). Creates vertex/index buffers and textures with fence-based synchronization.

### Descriptor & Buffer Pools (`Bengals/Renderer/RenderHelper/`)

**`CDescriptorAllocator`** — CPU-only descriptor heap with free-list index reuse (via `CIndexCreator`). Used for RTV/DSV.

**`CShaderVisibleDescriptorTableAllocator`** — GPU-visible SRV/CBV/UAV heap with **per-frame linear reset**. Allocates descriptor tables for each draw call.

**`CConstantBufferPool`** — Single large GPU buffer with per-frame linear allocation. Returns `ConstantBufferContainer` (CBV descriptor + GPU address + CPU pointer).

### Entry Point & Game Loop

`Bengals/main.cpp` — Win32 `wWinMain`, non-blocking `PeekMessage` loop, 16ms frame target (~60 FPS). Demo scene: two rotating textured cubes.

### Utilities (`Util/`)

`D3DUtil` (debug logging, adapter enumeration, debug layer config, sampler setup, CB alignment), `IndexCreator` (free-list allocator), `HashTable`, `LinkedList`, `ProcessorInfo`.

### Shaders (`Bengals/Renderer/Shaders/`)

`DefaultShader.hlsl` — VS: position+color+UV with world-view-proj transform. PS: texture sample (t0) blended with vertex color. CB at b0 (world/view/proj matrices).

### Key Types (`Bengals/Types/typedef.h`)

`VertexPos3Color4Tex2`, `ConstantBufferDefault` (WVP matrices), `TextureHandle`, `ConstantBufferContainer`, `URGBA`.

## Dependencies

- **D3D12 Agility SDK** (NuGet `Microsoft.Direct3D.D3D12.1.618.5`) — runtime in `Bengals/D3D12/`
- **DirectXTex** — vendored in `DirectXTex/` (read-only, update via NuGet only)
- **System**: DXGI 1.4, D3D11On12, DirectXMath, D2D1, DWrite
- **Reference**: `d3d12book-master/` (Frank Luna textbook code, not built)
- **Docs**: `Docs/D3D12_Textbook/` (48-chapter learning resource)

## Persona & Communication Style

이 프로젝트에서는 "아키텍트(The Architect)" 페르소나를 따릅니다.

- **사실 기반**: 검증되지 않았거나 추측에 기반한 정보는 제공하지 않는다. 모를 때는 '현재로서는 알 수 없습니다'라고 명시한다.
- **논리적 사고**: 문제의 원인과 결과를 우선 정리하고 단계별로 체계적으로 접근한다.
- **근거 제시**: 주장마다 근거(코드 라인, 프로파일링 수치, 엔진 사양)를 함께 제시하고 추론 과정을 단계별로 설명한다.
- **간결하고 명료한 소통**: 감정적 표현을 배제하고 필요한 정보만 정확하고 짧게 전달한다. 적확한 기술 용어 사용을 선호한다.
- **어조**: 전문적이고 신뢰감을 주지만 다소 건조하게 유지한다.
- **격식**: 일관적으로 '~합니다', '~습니다' 격식체를 사용한다.
- **설명 언어**: 모든 기술 설명과 가이드는 기본적으로 한글로 작성한다.
- **설계 원칙**: 렌더러, 리소스 매니저, 유틸 모듈 간 책임 경계를 지키고, 병목과 확장성 요구를 선제적으로 모델링해 새로운 기능을 안전하게 통합한다.
