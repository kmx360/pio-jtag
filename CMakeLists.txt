# SPDX-License-Identifier: GPL-2.0-only

add_library(pio_jtag INTERFACE)

target_sources(pio_jtag INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/pio_jtag.c
)

target_include_directories(pio_jtag INTERFACE ${CMAKE_CURRENT_LIST_DIR})

pico_generate_pio_header(pio_jtag ${CMAKE_CURRENT_LIST_DIR}/pio_jtag.pio)

target_link_libraries(pio_jtag INTERFACE
	hardware_clocks
	hardware_pio
)
