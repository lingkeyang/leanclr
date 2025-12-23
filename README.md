# LeanCLR

语言: [中文](./README.md) | [English](./README_EN.md)

LeanCLR 是一个面向全平台的精益 CLR（Common Language Runtime）实现。LeanCLR 的设计目标是在高度符合 ECMA-335 规范的前提下，提供更紧凑、易嵌入、低内存占用的运行时，实现对移动端、H5 与小游戏等资源受限平台的友好支持。

## 为什么是 LeanCLR

业界已有 CoreCLR、Mono、IL2CPP，为什么还需要 LeanCLR？出于以下考虑：

1. CoreCLR、Mono 体量较大，难以在极端资源场景下同时满足包体与内存占用目标。LeanCLR 设计紧凑，易于嵌入；单线程构建在 Win64 或 WebAssembly 平台约 **600 KB**。
1. IL2CPP 闭源，仅支持 AOT，对 ECMA-335 的完整度有限；在 H5 与小游戏发布场景中，wasm 二进制体积与加载后内存占用普遍偏高。
1. 相比 CoreCLR 代码基庞大、Mono 历史包袱较重且复杂，LeanCLR 代码更简洁清晰，便于定制与二次开发，以更好贴合具体业务需求。

## 特性与优势

- 高度兼容 ECMA-335，并支持部分 CoreCLR 对标准的扩展（如 .NET 7 起的“接口静态虚方法”）；移除部分过时设计（如 `arglist`、`jmp` 指令）。
- 实现极其紧凑：单线程版本在 Win64/wasm 约 **600 KB**；裁剪 IR 解释器及非必要 icall 后可进一步缩小。
- 原生且仅支持 **AOT + Interpreter** 混合执行模式：跨平台一致性好，契合移动/小游戏场景对包体与内存的要求。
- 内置 IL 与 IR 双解释器：低热函数用 IL 解释器，热点函数可切换到 IR 解释器，平衡 transform 开销与运行性能。
- 支持函数粒度 AOT，为包体与性能提供精细化权衡空间。
- 异常路径由解释器兜底，显著简化 AOT 代码体积。
- 更低的元数据（metadata）内存占用，支持函数体元数据的回收。
- 托管内存优化：单线程版本对象头仅一个指针大小。
- 按元数据/托管对象的对齐粒度使用独立分配池，避免 IL2CPP 统一 8 字节对齐造成的浪费。
- 代码结构清晰简洁，易读、易改、易优化。

## 项目状态

**代码预计在 2026-03-31 前开源，开发进度将持续更新。**

- 基本覆盖 ECMA-335；完整度低于 Mono，高于 `il2cpp + hybridclr`；仅部分实现 CoreCLR 扩展。
- 待办（TODO）：
  - GC
  - AOT 编译器（IL → C++）
  - P/Invoke
  - 多线程
  - 核心库（mscorlib）中平台相关 icall（如 `System.IO.File`）

## Demo

目前提供两个平台的演示（Demo）。

### Win64

目录位于 [demo/win64](./demo/win64)。

直接运行目录下的 `run.bat`，结束后可见类似输出：

```text
leanclr\demo\win64>lean -l dotnetframework CoreTests -e test.App::Main
[debugger][level:0][info] Hello, World!
ok!
```

`lean` 程序已嵌入 LeanCLR，可直接加载并运行 DLL。用法：

```bat
lean -l <dll search path> -e <entry point method> <main dll>

```

参数说明：

- `-l <dll search path>`：指定 DLL 搜索路径，可出现多次；默认搜索当前目录。
- `-e <entry point method>`：指定入口方法，格式为 `Namespace.Type::Method`，如 `-e test.App::Main`。
- `<main dll>`：指定要执行的主 DLL；其依赖的 DLL 将自动加载。

示例：

```bat
lean -l dotnetframework CoreTests -e test.App::Main
```

### HTML5

目录位于 [demo/h5](./demo/h5)。

该示例加载 `leanclr.wasm`，初始化 CLR，随后加载 `mscorlib`、`System.Core`、`CoreTests` 等 DLL，最终执行 `test.App::Main`。

使用方式：

- 启动一个 HTTP 服务器，站点根目录设为 `h5`。若已安装 npm，可在 `h5` 目录执行 `npx serve .` 启动本地服务。
- 在浏览器访问 `http://localhost:3000/`（若未使用 `npx serve`，请按实际端口调整）。
- 点击 “Load wasm” 按钮以加载并初始化 LeanCLR。
- 点击 “Run CoreTests::test.App::Main” 按钮以运行示例，可见输出 `Hello, World!`。

## 联系方式

- 邮箱：leanclr#code-philosophy.com
- QQ群：1047250380
