# AXI Timer Interrupt Demo - ZUBoard 1CG

Xilinx Vitis workspace demonstrating interrupt-driven programming on Zynq UltraScale+ MPSoC using AXI Timer peripherals.

## Hardware Platform

- **Board**: ZUBoard 1CG (xczu1cg)
- **CPU**: ARM Cortex-A53 (Standalone OS)
- **Peripherals**: AXI Timer (100 MHz clock)
- **Interrupt Controller**: GIC (Generic Interrupt Controller)

## Project Structure

```
vitis_irq/
├── platform2/                    # Hardware platform configuration
│   ├── hw/                       # Hardware design files
│   └── export/                   # BSP and platform exports
├── hello_world/                  # Original Xilinx example (reference)
│   └── src/
│       └── main.c                # Working xtmrctr_intr_example.c
├── hello_world2/                 # Custom interrupt implementation
│   └── src/
│       └── helloworld.c          # Timer interrupt demo
└── xtmrctr_intr_example/        # Additional example project
    └── src/
```

## Applications

### hello_world2
Custom AXI Timer interrupt demonstration that:
- Initializes AXI Timer with 100 MHz clock
- Configures timer for 1-second periodic interrupts
- Uses auto-reload and down-count mode
- Counts 10 interrupts then stops
- Demonstrates proper SDT platform interrupt setup

**Key Features:**
- Uses `XSetupInterruptSystem()` wrapper for SDT platforms
- Hardware register diagnostics for debugging
- Validates timer counting behavior
- Clean interrupt handler implementation

### hello_world
Reference Xilinx timer counter interrupt example (working baseline).

## Building the Project

### Prerequisites
- Xilinx Vitis 2025.1 or later
- ZUBoard 1CG hardware platform

### Build Steps

1. **Open Vitis IDE**
   ```bash
   vitis -workspace /path/to/vitis_irq
   ```

2. **Build Platform**
   - Right-click `platform2` → Build

3. **Build Application**
   - Right-click `hello_world2` → Build Project

4. **Program FPGA & Run**
   - Connect to ZUBoard via JTAG
   - Right-click `hello_world2` → Run As → Launch Hardware

## Expected Output

```
===================================
AXI TIMER INTERRUPT DEMO - ZUBoard 1CG
===================================
Timer Base Address: 0x80020000
Interrupt ID: 89
GIC Device ID: 0
Timer initialized successfully
Timer self-test passed
Interrupt system configured successfully
Timer handler registered
Timer options configured (INT + AUTO_RELOAD + DOWN_COUNT)
Timer reset value set to 0x05F5E100 (~1 sec @ 100 MHz)
Timer started - waiting for interrupts...
TCSR0 (Control/Status): 0x000000D0
Timer IS counting!

IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10

Timer stopped after 10 interrupts
Successfully ran Timer interrupt Example
```

## Key Configuration Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `TIMER_BASEADDR` | 0x80020000 | AXI Timer base address |
| `TIMER_INT_ID` | 89 | GIC interrupt ID |
| `RESET_VALUE` | 100000000 | Timer cycles for 1 second @ 100 MHz |
| `TIMER_CNTR_0` | 0 | Timer counter index |

## Technical Notes

### SDT Platform Interrupt Setup

Modern Xilinx platforms use **SDT (Software Defined Timer)** which requires:
- `XSetupInterruptSystem()` wrapper instead of manual GIC configuration
- Interrupt parameters from `TimerCounterInst.Config` structure
- Automatic GIC initialization and exception handling

### Timer Configuration

```c
XTmrCtr_SetOptions(&TimerCounterInst, 0,
    XTC_INT_MODE_OPTION |      // Enable interrupts
    XTC_AUTO_RELOAD_OPTION |   // Auto-reload on expire
    XTC_DOWN_COUNT_OPTION);    // Count down from load value
```

### Common Issues & Solutions

**Problem**: Timer counts up instead of down
- **Solution**: Add `XTC_DOWN_COUNT_OPTION` to timer options

**Problem**: Interrupts not firing
- **Solution**: Use `XSetupInterruptSystem()` for SDT platforms, not manual GIC setup

**Problem**: Wrong device ID compilation error
- **Solution**: Use `XPAR_XTMRCTR_0_BASEADDR` instead of `XPAR_TMRCTR_0_DEVICE_ID` for SDT platforms

## Hardware Design

The Vivado hardware design includes:
- Zynq UltraScale+ MPSoC PS
- AXI Timer IP (axi_timer_0)
- AXI Interconnect (axi_smc)
- AXI GPIO peripherals
- Processor System Reset module

Timer interrupt is routed through:
1. AXI Timer → Interrupt output
2. Zynq PS → PL-to-PS interrupt line
3. GIC → CPU interrupt handling

## References

- [Xilinx AXI Timer v2.0 LogiCORE IP Product Guide (PG079)](https://www.xilinx.com/support/documentation/ip_documentation/axi_timer/v2_0/pg079-axi-timer.pdf)
- [Zynq UltraScale+ MPSoC Technical Reference Manual (UG1085)](https://docs.xilinx.com/r/en-US/ug1085-zynq-ultrascale-trm)
- [Standalone OS BSP Documentation](https://xilinx.github.io/embeddedsw.github.io/standalone/doc/html/api/index.html)

## License

Based on Xilinx timer counter interrupt example.
Copyright © 2002-2024 Xilinx/AMD. All Rights Reserved.

## Authors

- Hardware Platform: ZUBoard 1CG
- Software: Adapted from Xilinx xtmrctr_intr_example.c
