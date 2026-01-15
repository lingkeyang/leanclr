# Win64 Demo

This directory provides a demonstration of LeanCLR on the Windows x64 platform.

## Quick Start

Run the `run.bat` script in this directory:

```cmd
run.bat
```

You should see output similar to:

```text
leanclr\demo\win64>lean -l dotnetframework CoreTests -e test.App::Main
[debugger][level:0][info] Hello, World!
ok!
```

## lean Tool Usage

`lean` is a command-line tool with the LeanCLR runtime embedded, capable of loading and executing .NET assemblies directly.

### Basic Syntax

```cmd
lean [options] <dll_name> [-- <dll_args>...]
```

### Options

| Option | Description |
|--------|-------------|
| `-l, --lib-dir <dir>` | Add a DLL search path (can be used multiple times); defaults to the current directory |
| `-e, --entry <entry>` | Specify entry method in the format `Namespace.Type::Method` |
| `-h, --help` | Display help information |
| `--` | Arguments after this are passed to the target DLL |

### Examples

```cmd
rem Run an assembly with default entry point
lean MyApp

rem Specify search paths and entry point
lean -l dotnetframework -l libs CoreTests -e test.App::Main

rem Pass arguments to the target DLL
lean -l . MyApp -- arg1 arg2 arg3
```

## Directory Structure

```
demo/win64/
├── run.bat              # Quick run script
├── lean.exe             # LeanCLR command-line tool
├── CoreTests.dll        # Test assembly
└── dotnetframework/     # .NET Framework base libraries
    ├── mscorlib.dll
    ├── System.dll
    └── System.Core.dll
```

## Testing Custom Code

### Compatibility Notes

- Only .NET Standard 2.x / .NET Framework 4.x core libraries are verified
- The `dotnetframework` directory contains only basic DLLs; use `-l <path>` to add additional search paths if needed

### Testing Workflow

1. Open [src/tests/managed/managed.sln](../../src/tests/managed/managed.sln) and modify `App::Main` or other code in the `CoreTests` project
2. Build to generate `CoreTests.dll` (Debug or Release)
3. Copy the output `CoreTests.dll` to this directory, replacing the original file
4. Run `run.bat` to verify the results

### Notes

- LeanCLR covers most of ECMA-335; complex C# code including exceptions and reflection will work as long as OS-specific APIs are not called
- Currently, only `System.Diagnostics.Debugger::Log` is available for logging output; do not use `Console.WriteLine` or `UnityEngine.Debug.Log`
