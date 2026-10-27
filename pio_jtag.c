// SPDX-License-Identifier: GPL-2.0-only

#include "hardware/clocks.h"
#include "pio_jtag.pio.h"

#include "pio_jtag.h"

bool pio_jtag_init(struct pio_jtag *jtag, const struct pio_jtag_pins *pins, uint32_t hz)
{
	uint lo = MIN(MIN(pins->tck, pins->tms), MIN(pins->tdi, pins->tdo));
	uint hi = MAX(MAX(pins->tck, pins->tms), MAX(pins->tdi, pins->tdo));

	if (!pio_claim_free_sm_and_add_program_for_gpio_range(&pio_jtag_program, &jtag->pio, &jtag->sm, &jtag->offset, lo, hi - lo + 1, true))
		return false;

	pio_jtag_program_init(jtag->pio, jtag->sm, jtag->offset,
		pins->tck, pins->tms, pins->tdi, pins->tdo);
	pio_jtag_set_freq(jtag, hz);
	pio_sm_set_enabled(jtag->pio, jtag->sm, true);
	return true;
}

void pio_jtag_deinit(struct pio_jtag *jtag)
{
	pio_sm_set_enabled(jtag->pio, jtag->sm, false);
	pio_remove_program_and_unclaim_sm(&pio_jtag_program, jtag->pio, jtag->sm, jtag->offset);
}

uint32_t pio_jtag_set_freq(struct pio_jtag *jtag, uint32_t hz)
{
	uint32_t sm_hz = clock_get_hz(clk_sys) / pio_jtag_CYCLES_PER_TCK;
	uint32_t div = hz ? (sm_hz + hz - 1) / hz : UINT16_MAX;

	if (div < 1)
		div = 1;
	if (div > UINT16_MAX)
		div = UINT16_MAX;

	pio_sm_set_clkdiv_int_frac8(jtag->pio, jtag->sm, div, 0);
	return sm_hz / div;
}

void __not_in_flash_func(pio_jtag_tms)(struct pio_jtag *jtag, uint32_t bits, unsigned n)
{
	pio_sm_put_blocking(jtag->pio, jtag->sm,
		pio_jtag_tms_cmd(jtag->offset, n));
	pio_sm_put_blocking(jtag->pio, jtag->sm, bits);
}

void __not_in_flash_func(pio_jtag_shift)(struct pio_jtag *jtag, const uint32_t *tdi, uint32_t *tdo, unsigned n)
{
	pio_sm_put_blocking(jtag->pio, jtag->sm,
		pio_jtag_shift_cmd(jtag->offset, n));

	// One word in, one out, so neither FIFO can fill up
	for (unsigned i = 0; i < (n + 31) / 32; ++i) {
		pio_sm_put_blocking(jtag->pio, jtag->sm, tdi ? tdi[i] : 0);
		uint32_t w = pio_sm_get_blocking(jtag->pio, jtag->sm);
		if (tdo)
			tdo[i] = w;
	}
}

// TMS sequences, sent LSB first, as "TMS bit -> next state"

// 1 -> ... (from anywhere, 5 times) -> Test-Logic-Reset, 0 -> Run-Test/Idle
#define TMS_RESET_TO_IDLE	0b011111, 6

// 1 -> Select-DR, 0 -> Capture-DR, 0 -> Shift-DR
#define TMS_IDLE_TO_SHIFT_DR	0b001, 3

// 1 -> Select-DR, 1 -> Select-IR, 0 -> Capture-IR, 0 -> Shift-IR
#define TMS_IDLE_TO_SHIFT_IR	0b0011, 4

// 1 -> Update-xR, 0 -> Run-Test/Idle
#define TMS_EXIT1_TO_IDLE	0b01, 2

void __not_in_flash_func(pio_jtag_reset)(struct pio_jtag *jtag)
{
	pio_jtag_tms(jtag, TMS_RESET_TO_IDLE);
}

void __not_in_flash_func(pio_jtag_idle)(struct pio_jtag *jtag, uint32_t n)
{
	while (n) {
		unsigned k = n < 32 ? n : 32;
		pio_jtag_tms(jtag, 0, k);
		n -= k;
	}
}

void __not_in_flash_func(pio_jtag_ir_scan)(struct pio_jtag *jtag, const uint32_t *tdi, uint32_t *tdo, unsigned n)
{
	pio_jtag_tms(jtag, TMS_IDLE_TO_SHIFT_IR);
	pio_jtag_shift(jtag, tdi, tdo, n);
	pio_jtag_tms(jtag, TMS_EXIT1_TO_IDLE);
}

void __not_in_flash_func(pio_jtag_dr_scan)(struct pio_jtag *jtag, const uint32_t *tdi, uint32_t *tdo, unsigned n)
{
	pio_jtag_tms(jtag, TMS_IDLE_TO_SHIFT_DR);
	pio_jtag_shift(jtag, tdi, tdo, n);
	pio_jtag_tms(jtag, TMS_EXIT1_TO_IDLE);
}
