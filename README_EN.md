# LeanCLR

Language: [中文](./README.md) | [English](./README_EN.md)

LeanCLR is a lean, cross-platform implementation of the Common Language Runtime (CLR). Its goal is to closely adhere to the ECMA-335 standard while providing a compact, embeddable, and memory-efficient runtime tailored for resource-constrained environments such as mobile, HTML5, and mini-game platforms.

## Why LeanCLR

CoreCLR, Mono, and IL2CPP are widely used; why build LeanCLR? Our motivations:

1. CoreCLR and Mono are relatively heavy, making it hard to simultaneously meet package size and memory constraints in extreme resource scenarios. LeanCLR is tightly designed and easy to embed; the single-thread build for Win64 or WebAssembly is about **600 KB**.
1. IL2CPP is closed-source and AOT-only, with limited ECMA-335 coverage. On H5 and mini-game distribution targets, wasm binary size and post-load memory usage are often high.
1. Compared with CoreCLR’s large codebase and Mono’s complexity and historical baggage, LeanCLR offers a cleaner, simpler codebase that is easier to customize and extend for specific needs.

## Features & Advantages

- High fidelity to ECMA-335 with support for selected CoreCLR extensions (e.g., interface static virtual methods introduced in .NET 7); deprecated constructs such as `arglist` and `jmp` are removed.
- Extremely compact: single-thread builds on Win64/wasm are around **600 KB**; further reductions are possible by trimming the IR interpreter and unnecessary icalls.
- Native and exclusive **AOT + Interpreter** hybrid execution mode: consistent cross-platform behavior and a good fit for mobile/mini-game requirements on package size and memory.
- Dual interpreters for IL and IR: use the IL interpreter for cold paths and switch hot paths to the IR interpreter to balance transform overhead with runtime performance.
- Function-level AOT granularity, giving developers fine-grained control to balance package size and performance.
- Exception paths are backed by the interpreter, significantly simplifying AOT code size.
- Lower metadata memory footprint with support for reclaiming function-body metadata.
- Optimized managed memory: in single-thread builds, the object header is only one pointer in size.
- Separate allocation pools based on metadata/managed object alignment granularity, avoiding the uniform 8-byte alignment overhead commonly seen with IL2CPP.
- Clean and simple code organization: easy to read, modify, and optimize.

## Project Status

**The code is expected to be open-sourced by 2026-03-31. Development progress will be updated regularly.**

- ECMA-335 coverage: less complete than Mono, but more complete than `il2cpp + hybridclr`; only a subset of CoreCLR extensions are implemented.
- TODO:
  - GC
  - AOT compiler (IL → C++)
  - P/Invoke
  - Multi-threading
  - Platform-specific icalls in the core library (mscorlib), e.g., `System.IO.File`

## Demo

We currently provide demos for two platforms.

### Win64

Directory: [demo/win64](./demo/win64)

Run `run.bat` in the directory. You should see output similar to:

```text
leanclr\demo\win64>lean -l dotnetframework CoreTests -e test.App::Main
[debugger][level:0][info] Hello, World!
ok!
```

The `lean` program embeds LeanCLR and can directly load and run DLLs. Usage:

```bat
lean -l <dll search path> -e <entry point method> <main dll>

```

Parameters:

- `-l <dll search path>`: Specify one or more DLL search paths; the current directory is searched by default.
- `-e <entry point method>`: Specify the entry method in the form `Namespace.Type::Method`, e.g., `-e test.App::Main`.
- `<main dll>`: Specify the main DLL to execute; its dependency DLLs will be loaded automatically.

Example:

```bat
lean -l dotnetframework CoreTests -e test.App::Main
```

### HTML5

Directory: [demo/h5](./demo/h5)

This demo loads `leanclr.wasm`, initializes the CLR, then loads `mscorlib`, `System.Core`, and `CoreTests`, and finally executes `test.App::Main`.

Usage:

- Start an HTTP server with the site root set to `h5`. If you have npm installed, in the `h5` directory run `npx serve .` to start a local server.
- Open your browser at `http://localhost:3000/` (adjust the URL if you are not using `npx serve`).
- Click the “Load wasm” button to load and initialize LeanCLR.
- Click “Run CoreTests::test.App::Main” to run the program; you should see `Hello, World!`.

## Contact

- Email: leanclr#code-philosophy.com
- QQ Group: 1047250380
