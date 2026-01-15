# LeanCLR

Language: [中文](./README.md) | [English](./README_EN.md)

[![GitHub](https://img.shields.io/badge/GitHub-Repository-181717?logo=github)](https://github.com/focus-creative-games/leanclr) [![Gitee](https://img.shields.io/badge/Gitee-Repository-C71D23?logo=gitee)](https://gitee.com/focus-creative-games/leanclr)

[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/focus-creative-games/leanclr/blob/main/LICENSE) [![DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/focus-creative-games/leanclr) [![Discord](https://img.shields.io/badge/Discord-Join-7289DA?logo=discord&logoColor=white)](https://discord.gg/esAYcM6RDQ)

LeanCLR is a lean, cross-platform CLR (Common Language Runtime) implementation. LeanCLR is designed to be highly compliant with the ECMA-335 specification while providing a more compact, embeddable, and low-memory-footprint runtime, offering friendly support for resource-constrained platforms such as mobile devices, H5, and mini-games.

## Why LeanCLR

There are already mature CLR implementations like CoreCLR, Mono, and IL2CPP. Why do we need LeanCLR?

### Limitations of Existing Solutions

| Runtime | Main Issues |
|---------|-------------|
| **CoreCLR** | Large footprint (tens of MB), high code complexity, difficult to trim and embed; not suitable for resource-constrained scenarios |
| **Mono** | Heavy legacy burden, complex architecture; still relatively large even after trimming; high maintenance and customization costs |
| **IL2CPP** | Closed source, AOT only; limited ECMA-335 completeness; high wasm binary size and memory usage |

### LeanCLR's Design Philosophy

LeanCLR is designed from scratch, focusing on the following core goals:

- **Ultra Compact** — Single-threaded version is only about **600 KB** on Win64/WebAssembly, can be trimmed down to **300 KB**
- **Easy to Embed** — Pure C++17 implementation with no external dependencies, easily integrates into any C++ project
- **Cross-Platform Consistency** — AOT + Interpreter hybrid mode, no JIT, ensuring consistent behavior across all platforms
- **Maintainability** — Clean code structure, easy to understand, customize, and extend

LeanCLR is particularly suitable for:

- 📱 Mobile games and applications (iOS/Android)
- 🌐 H5 games and mini-game platforms (WeChat Mini Games, TikTok Mini Games, etc.)
- 🎮 Game clients requiring hot-update capabilities
- 🔧 Embedded systems and IoT devices

## Features and Advantages

### Standards Compliance

- **Highly Compatible with ECMA-335** — Nearly complete CLI specification implementation, coverage exceeds `IL2CPP + HybridCLR`
- **Modern C# Feature Support** — Full support for generics, exception handling, reflection, delegates, LINQ, and other core features
- **CoreCLR Extension Support** (Planned) — Static abstract interface methods (.NET 7+) and other extension features
- **Lean Design** — Only removes deprecated features (such as `arglist`, `jmp` instructions)

### Ultra Lightweight

- **Minimal Footprint** — Single-threaded version is about **600 KB**; can be reduced to **300 KB** or less by trimming IR interpreter and non-essential icalls
- **Low Memory Usage** — Optimized metadata representation with on-demand method body metadata reclamation
- **Fine-Grained Alignment** — Uses separate allocation pools based on actual alignment requirements of metadata/managed objects, avoiding the waste of uniform 8-byte alignment
- **Compact Object Header** — Single-threaded version object header is only one pointer size

### Execution Modes

- **AOT + Interpreter Hybrid Execution** — Balances startup speed and runtime efficiency with consistent cross-platform behavior
- **Dual Interpreter Architecture** — IL interpreter handles cold paths, IR interpreter optimizes hot functions, balancing compilation overhead and execution performance
- **Function-Level AOT** — Supports per-function AOT or interpretation, enabling fine-grained trade-offs between binary size and performance
- **Exception Path Fallback** — Exception handling is uniformly handled by the interpreter, significantly reducing AOT code size

### Cross-Platform Capabilities

- **Pure C++ Implementation** — Based on C++17 standard with zero platform-specific dependencies
- **No Exception Mechanism Dependency** — Does not rely on C++ exceptions, can compile and run in environments with exceptions disabled
- **Zero Porting Cost** — Can be compiled directly to any platform supporting C++17 (Windows, Linux, macOS, iOS, Android, WebAssembly, etc.)

### Code Quality

- **Clear Structure** — Modular design with clear responsibilities, easy to understand and navigate
- **Easy to Customize** — Clean codebase makes it easy to trim and extend based on requirements
- **Easy to Optimize** — Clear execution paths make performance bottlenecks easy to identify and optimize

## Editions

LeanCLR provides two editions to meet different scenario requirements:

| Feature | Universal Edition | Standard Edition |
|---------|-------------------|------------------|
| **Threading Model** | Single-threaded | Multi-threaded |
| **Execution Mode** | AOT + Interpreter | AOT + Interpreter |
| **P/Invoke** | AOT functions only | AOT functions only |
| **Garbage Collection** | Precise cooperative GC | Conservative GC |
| **Platform icalls** | Not implemented | Fully implemented |
| **Platform Dependencies** | None | Requires platform adaptation |
| **Porting Cost** | Zero cost | Requires platform interface handling |
| **Use Cases** | WebAssembly, embedded, high cross-platform consistency | Desktop/mobile, full system functionality needed |

### Universal Edition

**Design Goal**: Ultimate cross-platform capability and portability

- **Single-Threaded Execution** — Simplified memory model, no thread synchronization concerns
- **AOT + Interpreter Hybrid Execution** — Flexible execution strategy
- **Precise Cooperative GC** — Precise tracking of managed references, efficient memory reclamation
- **Zero Platform Dependencies** — No reliance on any OS or platform-specific functions
- **Standard C++17 Implementation** — Can be compiled and run directly on any platform supporting C++17
- **Platform icalls Not Implemented** — Features like `System.IO.File` require custom bridging or pure managed implementations

**Best Use Cases**: WebAssembly, embedded systems, IoT devices, projects requiring high cross-platform consistency

### Standard Edition

**Design Goal**: Feature-complete production-grade runtime

- **Multi-Threading Support** — Complete threading model and synchronization primitives
- **AOT + Interpreter Hybrid Execution** — Flexible execution strategy
- **Conservative GC** — Better compatibility, suitable for complex application scenarios
- **Complete Platform icalls** — Implements `System.IO`, `System.Net`, and other platform-specific functionality
- **Standard C++17 Implementation** — Requires adaptation of a few platform-specific interfaces when porting to new platforms

**Best Use Cases**: Desktop applications, mobile games, projects requiring full .NET base library functionality

## Project Status

### Current Progress

| Module | Status | Description |
|--------|--------|-------------|
| **Metadata Parsing** | ✅ Complete | Full support for PE/COFF format and CLI metadata tables |
| **Type System** | ✅ Complete | Classes, interfaces, generics, arrays, value types, etc. |
| **IL Interpreter** | ✅ Complete | Covers almost all ECMA-335 IL instructions |
| **IR Interpreter** | ✅ Complete | Optimized execution for hot functions |
| **Exception Handling** | ✅ Complete | try/catch/finally, nested exceptions, etc. |
| **Reflection** | ✅ Complete | Type, MethodInfo, FieldInfo, and other core APIs |
| **Delegates** | ✅ Complete | Unicast/multicast delegates, generic delegates |
| **Internal Calls** | 🔶 In Progress | Core icalls implemented, platform icalls being added |
| **Garbage Collection** | 🔶 In Development | Basic framework ready |
| **AOT Compiler** | 📋 Planned | IL → C++ transpilation |
| **P/Invoke** | 📋 Planned | Native interop support |
| **Multi-Threading** | 📋 Planned | Threads, synchronization primitives, etc. |

### ECMA-335 Compatibility

- Completeness exceeds `IL2CPP + HybridCLR` combination
- Slightly lower completeness than Mono (main gap is platform-specific icalls)
- CoreCLR extension features (such as static abstract interface methods) will be implemented in future versions

### Roadmap

**Near-Term Goals:**

- Complete garbage collector implementation
- Implement AOT compiler (IL → C++)
- Support P/Invoke native interop
- Support CoreCLR extension features
- Provide more complete examples and documentation

**Mid-Term Goals:**

- Add more platform-specific internal calls (such as `System.IO`)
- Multi-threading support

**Long-Term Goals:**

- Continuous performance optimization
- Broader platform support

## Project Structure

```
leanclr/
├── src/
│   ├── runtime/      # LeanCLR runtime core
│   ├── libraries/    # Base class libraries
│   ├── tools/        # Command-line tools
│   ├── samples/      # Sample projects
│   └── tests/        # Unit tests
├── demo/             # Demo examples
└── tools/            # Build utilities
```

### runtime (Runtime Core)

Directory: `src/runtime`

Core implementation of the LeanCLR runtime, including:

- **metadata** - ECMA-335 metadata parsing and representation
- **vm** - Virtual machine core (type system, method invocation, runtime state management)
- **interp** - IL interpreter and IR interpreter implementation
- **gc** - Garbage collector (in development)
- **icalls** - Internal calls implementation
- **alloc** - Memory allocators (metadata allocation, managed object allocation)

### libraries (Base Class Libraries)

Directory: `src/libraries`

Contains .NET Framework base class libraries required by the runtime (mscorlib, System, System.Core, etc.).

### tools (Command-Line Tools)

Directory: `src/tools`

- **lean** - Command-line tool with embedded LeanCLR, can directly load and run .NET assemblies

### samples (Sample Projects)

Directory: `src/samples`

- **startup** - Win64 native platform sample project
- **lean-wasm** - WebAssembly platform sample project

## Documentation

Detailed documentation is available in the [docs](./docs) directory:

- [Documentation Overview](./docs/README.md) - Documentation structure and navigation
- [Build Documentation](./docs/build/README.md) - Build-related documentation overview
- [Building the Runtime](./docs/build/build_runtime.md) - How to build the LeanCLR runtime
- [Embedding LeanCLR](./docs/build/embed_leanclr.md) - How to integrate LeanCLR into your project
- [Test Framework](./src/tests/README.md) - Unit test framework and how to write test cases

## Quick Build

### Windows (Visual Studio)

```cmd
cd src/runtime
build.bat Release
```

### WebAssembly

```cmd
# 1. Prepare Emscripten SDK environment
emsdk_env.bat

# 2. Build
cd src/samples/lean-wasm
build-wasm.bat
```

For more details, see [Build Documentation](./docs/build/build_runtime.md).

## Demo

Two platform demos are provided for quickly experiencing LeanCLR's capabilities.

### Win64

Directory: [demo/win64](./demo/win64)

Run `run.bat` to execute the demo. See [demo/win64/README.md](./demo/win64/README.md) for details.

### HTML5

Directory: [demo/h5](./demo/h5)

Access `index.html` through an HTTP server to run the demo in a browser. See [demo/h5/README.md](./demo/h5/README.md) for details.

## Contact

- Email: leanclr#code-philosophy.com
- Discord: <https://discord.gg/esAYcM6RDQ>
- QQ Group: 1047250380
