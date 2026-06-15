/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <nrfx_spim.h>
#include <dmm.h>

#include <zephyr/drivers/counter.h>

/* SPI MODE 0 */

#define SPI_WAVEFORM_SPEC_MAX_FREQUENCIES 4

struct spi_waveform_spec {
	struct spi_dt_spec spi_spec;
	NRF_SPIM_Type *spim_reg;

	uint32_t frequencies[SPI_WAVEFORM_SPEC_MAX_FREQUENCIES];
	size_t num_frequencies;

	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
};

#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB)

#define TEST_BUFFER_SIZE 8
static uint8_t tx_buffer[TEST_BUFFER_SIZE] DMM_MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_slow_dt)));
static uint8_t rx_buffer[TEST_BUFFER_SIZE] DMM_MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_slow_dt)));

static uint8_t tx_buffer_fast[TEST_BUFFER_SIZE] DMM_MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_fast_dt)));
static uint8_t rx_buffer_fast[TEST_BUFFER_SIZE] DMM_MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi_fast_dt)));

static const struct spi_waveform_spec spim_slow_spec = {
	.spi_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_slow_dt), SPI_MODE),
	.spim_reg = (NRF_SPIM_Type *)DT_REG_ADDR(DT_NODELABEL(dut_spi_slow)),
	.frequencies = {MHZ(1), MHZ(4)},
	.num_frequencies = 2,
	.tx_buffer = tx_buffer,
	.rx_buffer = rx_buffer,
};

static const struct spi_waveform_spec spim_fast_spec = {
	.spi_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_fast_dt), SPI_MODE),
	.spim_reg = (NRF_SPIM_Type *)DT_REG_ADDR(DT_NODELABEL(dut_spi_fast)),
	.frequencies = {MHZ(4), MHZ(16)},
	.num_frequencies = 2,
	.tx_buffer = tx_buffer_fast,
	.rx_buffer = rx_buffer_fast,
};

static void generate_waveform(const struct spi_waveform_spec *waveform_spec);

int main(void)
{
	if (!spi_is_ready_dt(&spim_slow_spec.spi_spec)) {
		printk("SPIM device is not ready\n");
		return -ENODEV;
	}

#if IS_ENABLED(CONFIG_SPIM_WORKAROUND_FORCE_OFF)
	printk("SPIM workaround is disabled\n");
#else
	printk("SPIM workaround is enabled\n");
#endif

	generate_waveform(&spim_slow_spec);
	generate_waveform(&spim_fast_spec);
}

static void set_buffers(const struct spi_waveform_spec *waveform_spec)
{
	memset(waveform_spec->tx_buffer, 0x8B, TEST_BUFFER_SIZE);
	memset(waveform_spec->rx_buffer, 0xFF, TEST_BUFFER_SIZE);
}

static void generate_waveform(const struct spi_waveform_spec *waveform_spec)
{
	int err;

	struct spi_buf tx_spi_buf = {.buf = waveform_spec->tx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = waveform_spec->rx_buffer, .len = TEST_BUFFER_SIZE};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	set_buffers(waveform_spec);

	for (size_t i = 0; i < waveform_spec->num_frequencies; i++) {
		uint32_t frequency = waveform_spec->frequencies[i];
		printk("Generating waveform at %u Hz\n", frequency);

		struct spi_dt_spec spi_spec = waveform_spec->spi_spec;
		spi_spec.config.frequency = frequency;

		err = spi_transceive_dt(&spi_spec, &tx_spi_buf_set, &rx_spi_buf_set);
		if (err) {
			printk("SPI transceive failed: %d\n", err);
			return;
		}
	}
}
