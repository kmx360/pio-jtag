# pio_jtag

JTAG host for the RP2040/RP2350 on one PIO state machine.

- Any 4 GPIOs, no adjacency needed (within one 32 GPIO window on RP2350B)
- TCK = clk_sys / (5 * div), up to 30 MHz at 150 MHz

## Usage

```cmake
add_subdirectory(pio_jtag)
target_link_libraries(app pio_jtag)
```

```c
struct pio_jtag jtag;
struct pio_jtag_pins pins = { .tck = 11, .tms = 9, .tdi = 8, .tdo = 10 };
uint32_t ir = 0x12f01002, idcode;

pio_jtag_init(&jtag, &pins, 1000000);
pio_jtag_reset(&jtag);
pio_jtag_ir_scan(&jtag, &ir, NULL, 32);
pio_jtag_dr_scan(&jtag, NULL, &idcode, 32);
```

`pio_jtag_tms()` and `pio_jtag_shift()` give direct control of the TAP
state for anything the scan helpers don't cover.
