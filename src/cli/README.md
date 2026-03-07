# SimulIDE Headless CLI

SimulIDE can be run headlessly to automate circuit simulations and capture UART
serial output — useful for CI testing of MCU firmware.

## Installation

After building SimulIDE, install the `simulide-cli` wrapper script onto your `PATH`:

```sh
# macOS — copy the app bundle and CLI wrapper
rm -rf /Applications/simulide.app
cp -R build_XX/executables/SimulIDE_2.0.0-/simulide.app /Applications/
cp scripts/simulide-cli /usr/local/bin/simulide-cli
```

`simulide-cli` is a thin shell wrapper that locates the SimulIDE binary and
passes `-nogui` automatically. The binary itself can also be invoked directly:

```sh
simulide -nogui -circuit <file.sim2> [options]
```

## Usage

```
simulide-cli -circuit <path/to/circuit.sim2> [options]
```

### Options

| Flag | Description |
|------|-------------|
| `-circuit <file>` | Load a `.sim2` or `.sim1` circuit file *(required)* |
| `-firmware <file>` | Load firmware (`.hex`) into the first MCU in the circuit |
| `-out <file>` | Write UART TX output to a file (default: stdout) |
| `-stop-signal <string>` | Exit with code 0 when this string appears in UART output |
| `-idle-timeout <ms>` | Exit with code 1 if no UART output for this many ms (default: 5000) |

### Exit codes

| Code | Meaning |
|------|---------|
| `0` | Stop signal found in UART output |
| `1` | Idle timeout elapsed, or a startup error occurred |

## Examples

Run a circuit and print serial output to stdout:

```sh
simulide-cli -circuit myproject.sim2 -firmware firmware.hex
```

Run firmware tests and exit when the test suite reports completion:

```sh
simulide-cli -circuit myproject.sim2 -firmware firmware.hex \
    -stop-signal "All tests passed!" -out results.txt
```

## Implementation notes

- **`HeadlessCli`** (`headlesscli.h/.cpp`) — parses CLI flags, validates inputs,
  configures `UsartCapture`, and orchestrates circuit load / firmware flash /
  power-on after the Qt event loop starts.
- **`UsartCapture`** (`usartcapture.h/.cpp`) — thread-safe singleton hooked into
  `UartTx::runEvent()` in `src/microsim/modules/usart/usarttx.cpp`. Writes each
  transmitted byte to a file or stdout, checks for the stop-signal sentinel, and
  drives the idle-timeout logic.
- The process exits via `std::_Exit()` (bypassing Qt teardown) to avoid a
  segfault caused by a simulator-thread race during `QCoreApplication::exit()`.
