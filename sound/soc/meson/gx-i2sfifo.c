// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/soc.h>


static int gx_i2sfifo_component_probe(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_i2sfifo_component_probe");
	return 0;
}


static void gx_i2sfifo_component_remove(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_i2sfifo_component_remove");
}


static int gx_i2sfifo_component_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2sfifo_component_open");
	return 0;
}


static int gx_i2sfifo_component_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2sfifo_component_close");
	return 0;
}


static int gx_i2sfifo_component_pcm_construct(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	dev_dbg(component->dev, "gx_i2sfifo_pcm_construct");
	return 0;
}


static void gx_i2sfifo_component_pcm_destruct(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	dev_dbg(component->dev, "gx_i2sfifo_pcm_destruct");
}


snd_pcm_uframes_t gx_i2sfifo_component_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	return 0;
}


static const struct snd_soc_component_driver gx_i2sfifo_component_driver = {
	.name = "I2SFIFO",
	.probe = gx_i2sfifo_component_probe,
	.remove = gx_i2sfifo_component_remove,
	.open = gx_i2sfifo_component_open,
	.close = gx_i2sfifo_component_close,
	.pcm_construct = gx_i2sfifo_component_pcm_construct,
	.pcm_destruct = gx_i2sfifo_component_pcm_destruct,
	.pointer = gx_i2sfifo_component_pointer,
};


static int gx_i2sfifo_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_startup");
	return 0;
}


static void gx_i2sfifo_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_shutdown");
}


static int gx_i2sfifo_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_prepare");
	return 0;
}


static int gx_i2sfifo_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_trigger");
	return 0;
}


static int gx_i2sfifo_dai_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_hw_params");
	return 0;
}


static const struct snd_soc_dai_ops gx_i2sfifo_dai_ops = {
	.startup = gx_i2sfifo_dai_startup,
	.shutdown = gx_i2sfifo_dai_shutdown,
	.prepare = gx_i2sfifo_dai_prepare,
	.trigger = gx_i2sfifo_dai_trigger,
	.hw_params = gx_i2sfifo_dai_hw_params,
};


static int gx_i2sfifo_dai_probe(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_probe");
	return 0;
}


static int gx_i2sfifo_dai_remove(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_i2sfifo_dai_remove");
	return 0;
}


static struct snd_soc_dai_driver gx_i2sfifo_dai_driver[] = {
	{
		.name = "I2SFIFO",
		.probe = gx_i2sfifo_dai_probe,
		.remove = gx_i2sfifo_dai_remove,
		.ops = &gx_i2sfifo_dai_ops,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	},
};


static int gx_i2sfifo_platform_probe(struct platform_device *pdev)
{
	int ret;

	dev_dbg(&pdev->dev, "gx_i2sfifo_platform_probe");

	ret = snd_soc_register_component(&pdev->dev, &gx_i2sfifo_component_driver, gx_i2sfifo_dai_driver, ARRAY_SIZE(gx_i2sfifo_dai_driver));
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to register I2SFIFO component");
		return ret;
	}

	return 0;
}


static int gx_i2sfifo_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_i2sfifo_platform_remove");

	snd_soc_unregister_component(&pdev->dev);

	return 0;
}


static const struct of_device_id gx_i2sfifo_of_match[] = {
	{ .compatible = "amlogic,gx-i2sfifo" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_i2sfifo_of_match);


static struct platform_driver gx_i2sfifo_platform_driver = {
	.driver = {
		.name = "gx-i2sfifo",
		.of_match_table = gx_i2sfifo_of_match,
	},
	.probe = gx_i2sfifo_platform_probe,
	.remove = gx_i2sfifo_platform_remove,
};
module_platform_driver(gx_i2sfifo_platform_driver);


MODULE_DESCRIPTION("Amlogic GX I2SFIFO audio driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
