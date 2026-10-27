// SPDX-License-Identifier: GPL-2.0-only

// JTAG host on one PIO state machine, any 4 GPIOs (in one 32 GPIO window)
//
// TCK = clk_sys / (5 * div)

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hardware/pio.h"

struct pio_jtag_pins {
	uint tck;
	uint tms;
	uint tdi;
	uint tdo;
};

struct pio_jtag {
	PIO pio;
	uint sm;
	uint offset;
};

// Claim a state machine and start at hz, false if no PIO is free
bool pio_jtag_init(struct pio_jtag *jtag, const struct pio_jtag_pins *pins, uint32_t hz);

// Release the state machine and program space
void pio_jtag_deinit(struct pio_jtag *jtag);

// Set TCK to at most hz, returns the actual frequency
uint32_t pio_jtag_set_freq(struct pio_jtag *jtag, uint32_t hz);

// Clock out n (1..32) TMS bits
void pio_jtag_tms(struct pio_jtag *jtag, uint32_t bits, unsigned n);

// Shift-xR -> Exit1-xR, n <= 2^22 bits. NULL tdi shifts zeros, NULL tdo discards.
void pio_jtag_shift(struct pio_jtag *jtag, const uint32_t *tdi, uint32_t *tdo, unsigned n);

// Any state -> Run-Test/Idle
void pio_jtag_reset(struct pio_jtag *jtag);

// Clock n times in Run-Test/Idle
void pio_jtag_idle(struct pio_jtag *jtag, uint32_t n);

// Run-Test/Idle -> Run-Test/Idle
void pio_jtag_ir_scan(struct pio_jtag *jtag, const uint32_t *tdi, uint32_t *tdo, unsigned n);
void pio_jtag_dr_scan(struct pio_jtag *jtag, const uint32_t *tdi, uint32_t *tdo, unsigned n);
