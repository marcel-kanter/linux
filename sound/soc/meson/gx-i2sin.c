// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 - 2023 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>


static int gx_i2sin_component_probe(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_i2sin_component_probe");
	return 0;
}


static void gx_i2sin_component_remove(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_i2sin_component_remove");
}


static int gx_i2sin_component_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2sin_component_open");
	return 0;
}


static int gx_i2sin_component_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2sin_component_close");
	return 0;
}


static const struct snd_soc_component_driver gx_i2sin_component_driver = {
	.name = "I2SIN",
	.probe = gx_i2sin_component_probe,
	.remove = gx_i2sin_component_remove,
	.open = gx_i2sin_component_open,
	.close = gx_i2sin_component_close,
};


static int gx_i2sin_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_startup");
	return 0;
}


static void gx_i2sin_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_shutdown");
}


static int gx_i2sin_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_prepare");
	return 0;
}


static int gx_i2sin_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_trigger");
	return 0;
}


static int gx_i2sin_dai_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_hw_params");
	return 0;
}


static int gx_i2sin_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_hw_free");
	return 0;
}


static int gx_i2sin_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_set_sysclk");
	return 0;
}


static int gx_i2sin_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_set_fmt");
	return 0;
}


static const struct snd_soc_dai_ops gx_i2sin_dai_ops = {
	.startup = gx_i2sin_dai_startup,
	.shutdown = gx_i2sin_dai_shutdown,
	.prepare = gx_i2sin_dai_prepare,
	.trigger = gx_i2sin_dai_trigger,
	.hw_params = gx_i2sin_dai_hw_params,
	.hw_free = gx_i2sin_dai_hw_free,
	.set_sysclk = gx_i2sin_dai_set_sysclk,
	.set_fmt = gx_i2sin_dai_set_fmt,
};


static int gx_i2sin_dai_probe(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_probe");
	return 0;
}


static int gx_i2sin_dai_remove(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sin_dai_remove");
	return 0;
}


static struct snd_soc_dai_driver gx_i2sin_dai_driver[] = {
	{
		.name = "I2SIN",
		.probe = gx_i2sin_dai_probe,
		.remove = gx_i2sin_dai_remove,
		.ops = &gx_i2sin_dai_ops,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	},
};


static int gx_i2sin_platform_probe(struct platform_device *pdev)
{
	int ret;

	dev_dbg(&pdev->dev, "gx_i2sin_platform_probe");

	ret = snd_soc_register_component(&pdev->dev, &gx_i2sin_component_driver, gx_i2sin_dai_driver, ARRAY_SIZE(gx_i2sin_dai_driver));
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to register I2SIN component: %d", ret);
		return ret;
	}

	return 0;
}


static int gx_i2sin_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_i2sin_platform_remove");

	snd_soc_unregister_component(&pdev->dev);

	return 0;
}


static const struct of_device_id gx_i2sin_of_match[] = {
	{ .compatible = "amlogic,gx-i2sin" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_i2sin_of_match);


static struct platform_driver gx_i2sin_platform_driver = {
	.driver = {
		.name = "gx-i2sin",
		.of_match_table = gx_i2sin_of_match,
	},
	.probe = gx_i2sin_platform_probe,
	.remove = gx_i2sin_platform_remove,
};
module_platform_driver(gx_i2sin_platform_driver);


MODULE_DESCRIPTION("Amlogic GX I2S input audio driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
