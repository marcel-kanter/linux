// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/soc.h>

#include "gx-audio.h"


static int gx_i2s_component_probe(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_i2s_component_probe");
	return 0;
}


static void gx_i2s_component_remove(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_i2s_component_remove");
}


static int gx_i2s_component_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2s_component_open");
	return 0;
}


static int gx_i2s_component_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2s_component_close");
	return 0;
}


static const struct snd_soc_component_driver gx_i2s_component_driver = {
	.name = "I2S",
	.probe = gx_i2s_component_probe,
	.remove = gx_i2s_component_remove,
	.open = gx_i2s_component_open,
	.close = gx_i2s_component_close,
};


static int gx_i2s_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2s_dai_startup");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	ret = clk_bulk_prepare_enable(gx_audio->aiu_i2s_num_clocks, gx_audio->aiu_i2s_clocks);
	if (ret)
	{
		dev_err(dai->dev, "Failed to enable I2S clocks: %d", ret);
		return ret;
	}

	return 0;
}


static void gx_i2s_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2s_dai_shutdown");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	clk_bulk_disable_unprepare(gx_audio->aiu_i2s_num_clocks, gx_audio->aiu_i2s_clocks);
}


static int gx_i2s_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2s_dai_prepare");
	return 0;
}


static int gx_i2s_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2s_dai_trigger");
	return 0;
}


static int gx_i2s_dai_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2s_dai_hw_params");
	return 0;
}


static int gx_i2s_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2s_dai_hw_free");
	return 0;
}


static int gx_i2s_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2s_dai_set_sysclk");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	if (clk_id != 0)
	{
		dev_err(dai->dev, "Unsupported clock id: %d", clk_id);
		return -EINVAL;
	}

	if (dir != SND_SOC_CLOCK_OUT)
	{
		return 0;
	}

	ret = clk_set_rate(gx_audio->aiu_i2s_clocks[AIU_CLK_I2S_MCLK].clk, freq);
	if (ret)
	{
		dev_err(dai->dev, "Failed to set sysclk to %uHz: %d", freq, ret);
	}

	return ret;
}


static int gx_i2s_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	dev_dbg(dai->dev, "gx_i2s_dai_set_fmt");
	return 0;
}


static const struct snd_soc_dai_ops gx_i2s_dai_ops = {
	.startup = gx_i2s_dai_startup,
	.shutdown = gx_i2s_dai_shutdown,
	.prepare = gx_i2s_dai_prepare,
	.trigger = gx_i2s_dai_trigger,
	.hw_params = gx_i2s_dai_hw_params,
	.hw_free = gx_i2s_dai_hw_free,
	.set_sysclk = gx_i2s_dai_set_sysclk,
	.set_fmt = gx_i2s_dai_set_fmt,
};


static int gx_i2s_dai_probe(struct snd_soc_dai *dai)
{
       dev_dbg(dai->dev, "gx_i2s_dai_probe");
       return 0;
}


static int gx_i2s_dai_remove(struct snd_soc_dai *dai)
{
       dev_dbg(dai->dev, "gx_i2s_dai_remove");
       return 0;
}


static struct snd_soc_dai_driver gx_i2s_dai_driver[] = {
	{
		.name = "I2S",
		.probe = gx_i2s_dai_probe,
		.remove = gx_i2s_dai_remove,
		.ops = &gx_i2s_dai_ops,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	},
};


static int gx_i2s_platform_probe(struct platform_device *pdev)
{
	int ret;

	dev_dbg(&pdev->dev, "gx_i2s_platform_probe");

	ret = snd_soc_register_component(&pdev->dev, &gx_i2s_component_driver, gx_i2s_dai_driver, ARRAY_SIZE(gx_i2s_dai_driver));
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to register I2S component: %d", ret);
		return ret;
	}

	return 0;
}


static int gx_i2s_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_i2s_platform_remove");

	snd_soc_unregister_component(&pdev->dev);

	return 0;
}


static const struct of_device_id gx_i2s_of_match[] = {
	{ .compatible = "amlogic,gx-i2s" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_i2s_of_match);


static struct platform_driver gx_i2s_platform_driver = {
	.driver = {
		.name = "gx-i2s",
		.of_match_table = gx_i2s_of_match,
	},
	.probe = gx_i2s_platform_probe,
	.remove = gx_i2s_platform_remove,
};
module_platform_driver(gx_i2s_platform_driver);


MODULE_DESCRIPTION("Amlogic GX I2S audio driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
