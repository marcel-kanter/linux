/* SPDX-License-Identifier: GPL-2.0 OR MIT */
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#ifndef _MESON_GX_AUDIO_H
#define _MESON_GX_AUDIO_H

#include <linux/clk.h>
#include <linux/regmap.h>


#define AIU_CLK_I2S_PCLK 0
#define AIU_CLK_I2S_AOCLK 1
#define AIU_CLK_I2S_MCLK 2
#define AIU_CLK_I2S_MIXER 3

#define AIU_CLK_SPDIF_PCLK 0
#define AIU_CLK_SPDIF_AOCLK 1
#define AIU_CLK_SPDIF_MCLK 2
#define AIU_CLK_SPDIF_MCLK_SEL 3

#define AUDIN_CLK_AUDIN_BUFCLK 0
#define AUDIN_CLK_AUDIN_CLK 1
#define AUDIN_CLK_ADC_CLK 2


struct gx_audio
{
	struct clk *pclk;

	struct regmap *aiu_regmap;

	struct clk_bulk_data *aiu_i2s_clocks;
	unsigned int aiu_i2s_num_clocks;

	struct clk_bulk_data *aiu_spdif_clocks;
	unsigned int aiu_spdif_num_clocks;

	struct regmap *audin_regmap;

	struct clk_bulk_data *audin_clocks;
	unsigned int audin_num_clocks;
};

#endif
