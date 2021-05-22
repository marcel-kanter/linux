// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/reset.h>

#include "gx-audio.h"


static const char * const aiu_i2s_clock_ids[] = {
	[AIU_CLK_I2S_PCLK] = "i2s_pclk",
	[AIU_CLK_I2S_AOCLK] = "i2s_aoclk",
	[AIU_CLK_I2S_MCLK] = "i2s_mclk",
	[AIU_CLK_I2S_MIXER] = "i2s_mixer",
};


static const char * const aiu_spdif_clock_ids[] = {
	[AIU_CLK_SPDIF_PCLK] = "spdif_pclk",
	[AIU_CLK_SPDIF_AOCLK] = "spdif_aoclk",
	[AIU_CLK_SPDIF_MCLK] = "spdif_mclk",
	[AIU_CLK_SPDIF_MCLK_SEL] = "spdif_mclk_sel",
};


static const char * const audin_clock_ids[] = {
	[AUDIN_CLK_AUDIN_BUFCLK] = "audin_bufclk",
	[AUDIN_CLK_AUDIN_CLK] = "audin_clk",
	[AUDIN_CLK_ADC_CLK] = "adc_clk",
};


static int gx_audio_clocks_get(struct platform_device *pdev)
{
	int ret;
	struct gx_audio *gx_audio;
	int i;

	gx_audio = platform_get_drvdata(pdev);

	gx_audio->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(gx_audio->pclk))
	{
		dev_err(&pdev->dev, "Failed to get the peripheral clock");
		return PTR_ERR(gx_audio->pclk);
	}

	gx_audio->aiu_i2s_clocks = devm_kcalloc(&pdev->dev, ARRAY_SIZE(aiu_i2s_clock_ids), sizeof(*gx_audio->aiu_i2s_clocks), GFP_KERNEL);
	if (!gx_audio->aiu_i2s_clocks)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for AIU I2S clock data structure");
		return -ENOMEM;
	}
	gx_audio->aiu_i2s_num_clocks = ARRAY_SIZE(aiu_i2s_clock_ids);

	i = 0;
	while (i < ARRAY_SIZE(aiu_i2s_clock_ids))
	{
		gx_audio->aiu_i2s_clocks[i].id = aiu_i2s_clock_ids[i];
		i++;
	}

	ret = devm_clk_bulk_get(&pdev->dev, ARRAY_SIZE(aiu_i2s_clock_ids), gx_audio->aiu_i2s_clocks);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to get the AIU I2S clocks");
		return ret;
	}

	gx_audio->aiu_spdif_clocks = devm_kcalloc(&pdev->dev, ARRAY_SIZE(aiu_spdif_clock_ids), sizeof(*gx_audio->aiu_spdif_clocks), GFP_KERNEL);
	if (!gx_audio->aiu_spdif_clocks)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for AIU SPDIF clock data structure");
		return -ENOMEM;
	}
	gx_audio->aiu_spdif_num_clocks = ARRAY_SIZE(aiu_spdif_clock_ids);

	i = 0;
	while (i < ARRAY_SIZE(aiu_spdif_clock_ids))
	{
		gx_audio->aiu_spdif_clocks[i].id = aiu_spdif_clock_ids[i];
		i++;
	}

	ret = devm_clk_bulk_get(&pdev->dev, ARRAY_SIZE(aiu_spdif_clock_ids), gx_audio->aiu_spdif_clocks);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to get the AIU SPDIF clocks");
		return ret;
	}

	gx_audio->audin_clocks = devm_kcalloc(&pdev->dev, ARRAY_SIZE(audin_clock_ids), sizeof(*gx_audio->audin_clocks), GFP_KERNEL);
	if (!gx_audio->audin_clocks)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for AUDIN clock data structure");
		return -ENOMEM;
	}
	gx_audio->audin_num_clocks = ARRAY_SIZE(audin_clock_ids);

	i = 0;
	while (i < ARRAY_SIZE(audin_clock_ids))
	{
		gx_audio->audin_clocks[i].id = audin_clock_ids[i];
		i++;
	}

	ret = devm_clk_bulk_get(&pdev->dev, ARRAY_SIZE(audin_clock_ids), gx_audio->audin_clocks);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to get the AUDIN clocks");
		return ret;
	}

	ret = clk_prepare_enable(gx_audio->pclk);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to enable peripheral clock");
		return ret;
	}

	return 0;
}


static int gx_audio_peripherals_reset(struct platform_device *pdev)
{
	int ret;
	struct reset_control *rst_ctrl;

	rst_ctrl = reset_control_get_exclusive(&pdev->dev, "aiu");
	if (IS_ERR(rst_ctrl))
	{
		dev_err(&pdev->dev, "Failed to get reset control for AIU");
		return PTR_ERR(rst_ctrl);
	}
	ret = reset_control_reset(rst_ctrl);
	reset_control_put(rst_ctrl);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to reset AIU");
		return ret;
	}

	rst_ctrl = reset_control_get_exclusive(&pdev->dev, "audin");
	if (IS_ERR(rst_ctrl))
	{
		dev_err(&pdev->dev, "Failed to get reset control for AUDIN");
		return PTR_ERR(rst_ctrl);
	}
	ret = reset_control_reset(rst_ctrl);
	reset_control_put(rst_ctrl);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to reset AUDIN");
		return ret;
	}

	return 0;
}


static int gx_audio_platform_probe(struct platform_device *pdev)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(&pdev->dev, "gx_audio_platform_probe");

	gx_audio = devm_kzalloc(&pdev->dev, sizeof(*gx_audio), GFP_KERNEL);
	if (!gx_audio)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for audio data structure");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, gx_audio);

	ret = gx_audio_clocks_get(pdev);
	if (ret)
	{
		return ret;
	}

	ret = gx_audio_peripherals_reset(pdev);
	if (ret)
	{
		return ret;
	}

	return 0;
}


static int gx_audio_platform_remove(struct platform_device *pdev)
{
	struct gx_audio *gx_audio;

	dev_dbg(&pdev->dev, "gx_audio_platform_remove");

	gx_audio = platform_get_drvdata(pdev);

	clk_disable_unprepare(gx_audio->pclk);

	return 0;
}


static const struct of_device_id gx_audio_of_match[] = {
	{ .compatible = "amlogic,gx-audio" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_audio_of_match);


static struct platform_driver gx_audio_platform_driver = {
	.driver = {
		.name = "gx-audio",
		.of_match_table = gx_audio_of_match,
	},
	.probe = gx_audio_platform_probe,
	.remove = gx_audio_platform_remove,
};
module_platform_driver(gx_audio_platform_driver);


MODULE_DESCRIPTION("Amlogic GX audio driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
