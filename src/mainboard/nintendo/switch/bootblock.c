/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/io.h>
#include <bootblock_common.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <soc/addressmap.h>
#include <soc/clk_rst.h>
#include <soc/clock.h>
#include <soc/funitcfg.h>
#include <soc/nvidia/tegra/i2c.h>
#include <soc/padconfig.h>
#include <soc/pmc.h>
#include <soc/power.h>
#include <soc/spi.h>

#include "pmic.h"

/********************** PMIC **********************************/
static const struct pad_config pmic_pads[] = {
	PAD_CFG_SFIO(PWR_I2C_SCL, PINMUX_INPUT_ENABLE, I2CPMU),
	PAD_CFG_SFIO(PWR_I2C_SDA, PINMUX_INPUT_ENABLE, I2CPMU),
};

/********************* Funits *********************************/
static const struct funit_cfg funits[] = {
	/* PMIC on I2C5 (PWR_I2C* pads) at 400kHz. */
	FUNIT_CFG(I2C5, PLLP, 400, pmic_pads, ARRAY_SIZE(pmic_pads)),
};

/********************* UART ***********************************/
static const struct pad_config uarta_pads[] = {
	/* UARTA: tx, rx, rts, cts */
	PAD_CFG_SFIO(UART1_TX, PINMUX_PULL_NONE, UARTA),
	PAD_CFG_SFIO(UART1_RX, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTA),
	PAD_CFG_SFIO(UART1_RTS, PINMUX_PULL_NONE, UARTA),
	PAD_CFG_SFIO(UART1_CTS, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTA),
};

static const struct pad_config uartb_pads[] = {
	/* UARTB: tx, rx, rts, cts */
	PAD_CFG_SFIO(UART2_TX, PINMUX_PULL_NONE, UARTB),
	PAD_CFG_SFIO(UART2_RX, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTB),
	PAD_CFG_SFIO(UART2_RTS, PINMUX_PULL_NONE, UARTB),
	PAD_CFG_SFIO(UART2_CTS, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTB),
};

static const struct pad_config uartc_pads[] = {
	/* UARTC: tx, rx, rts, cts */
	PAD_CFG_SFIO(UART3_TX, PINMUX_PULL_NONE, UARTC),
	PAD_CFG_SFIO(UART3_RX, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTC),
	PAD_CFG_SFIO(UART3_RTS, PINMUX_PULL_NONE, UARTC),
	PAD_CFG_SFIO(UART3_CTS, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTC),
};

static const struct pad_config uartd_pads[] = {
	/* UARTD: tx, rx, rts, cts */
	PAD_CFG_SFIO(UART4_TX, PINMUX_PULL_NONE, UARTD),
	PAD_CFG_SFIO(UART4_RX, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTD),
	PAD_CFG_SFIO(UART4_RTS, PINMUX_PULL_NONE, UARTD),
	PAD_CFG_SFIO(UART4_CTS, PINMUX_INPUT_ENABLE | PINMUX_PULL_UP, UARTD),
};

void bootblock_mainboard_early_init(void)
{
	switch (CONFIG_CONSOLE_SERIAL_TEGRA210_UART_ID) {
	case 0:
		soc_configure_pads(uarta_pads, ARRAY_SIZE(uarta_pads));
		break;
	case 1:
		soc_configure_pads(uartb_pads, ARRAY_SIZE(uartb_pads));
		break;
	case 2:
		soc_configure_pads(uartc_pads, ARRAY_SIZE(uartc_pads));
		break;
	case 3:
		soc_configure_pads(uartd_pads, ARRAY_SIZE(uartd_pads));
		break;
	}
}

static void set_clock_sources(void)
{
	void *rst_reg;
	uint32_t lclks, hclks, uclks;

	lclks = CLK_L_UARTA | CLK_L_UARTB;
	hclks = CLK_H_UARTC;
	uclks =  CLK_U_UARTD | CLK_U_UARTE;

	switch (CONFIG_CONSOLE_SERIAL_TEGRA210_UART_ID) {
	case 0:
		rst_reg = CLK_RST_REG(clk_src_uarta);
		lclks &= ~CLK_L_UARTA;
		break;
	case 1:
		rst_reg = CLK_RST_REG(clk_src_uartb);
		lclks &= ~CLK_L_UARTB;
		break;
	case 2:
		rst_reg = CLK_RST_REG(clk_src_uartc);
		hclks &= ~CLK_H_UARTC;
		break;
	case 3:
		rst_reg = CLK_RST_REG(clk_src_uartd);
		uclks &= ~CLK_U_UARTD;
		break;
	case 4:
		rst_reg = CLK_RST_REG(clk_src_uarte);
		uclks &= ~CLK_U_UARTE;
		break;
	default:
		/* none or invalid */
		return;
	}

	/*
	 * Take all but the Console UARTs (already done in clock_early_uart())
	 * out of reset
	 */
	clock_enable_clear_reset(lclks, hclks, uclks, 0, 0, 0, 0);

	/* Console UART gets PLLP, deactivate CLK_UART_DIV_OVERRIDE */
	write32(rst_reg, PLLP << CLK_SOURCE_SHIFT);
}

void bootblock_mainboard_init(void)
{
	struct tegra_i2c_bus_info *info = &tegra_i2c_info[I2CPWR_BUS];

	set_clock_sources();

	soc_configure_funits(funits, ARRAY_SIZE(funits));

	/* PMIC */
	info->reset_func(info->reset_bit);
	i2c_init(I2CPWR_BUS);
	pmic_init(I2CPWR_BUS);

	/*
	 * Set power detect override for GPIO, audio & sdmmc3 rails.
	 * GPIO rail override is required to put it into 1.8V mode.
	 */
	pmc_override_pwr_det(PMC_GPIO_RAIL_AO_MASK | PMC_AUDIO_RAIL_AO_MASK |
			     PMC_SDMMC3_RAIL_AO_MASK, PMC_GPIO_RAIL_AO_DISABLE |
			     PMC_AUDIO_RAIL_AO_DISABLE |
			     PMC_SDMMC3_RAIL_AO_DISABLE);
}
