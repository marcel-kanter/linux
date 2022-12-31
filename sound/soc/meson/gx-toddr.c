// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2022 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/soc.h>


static int gx_toddr_component_probe(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_toddr_component_probe");
	return 0;
}


static void gx_toddr_component_remove(struct snd_soc_component *component)
{
	dev_dbg(component->dev, "gx_toddr_component_remove");
}


static int gx_toddr_component_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_toddr_component_open");
	return 0;
}


static int gx_toddr_component_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_toddr_component_close");
	return 0;
}


static int gx_toddr_component_pcm_construct(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	dev_dbg(component->dev, "gx_toddr_pcm_construct");
	return 0;
}


static void gx_toddr_component_pcm_destruct(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	dev_dbg(component->dev, "gx_toddr_pcm_destruct");
}


snd_pcm_uframes_t gx_toddr_component_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	return 0;
}


static const struct snd_soc_component_driver gx_toddr_component_driver = {
	.name = "TODDR",
	.probe = gx_toddr_component_probe,
	.remove = gx_toddr_component_remove,
	.open = gx_toddr_component_open,
	.close = gx_toddr_component_close,
	.pcm_construct = gx_toddr_component_pcm_construct,
	.pcm_destruct = gx_toddr_component_pcm_destruct,
	.pointer = gx_toddr_component_pointer,
};


static int gx_toddr_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_startup");
	return 0;
}


static void gx_toddr_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_shutdown");
}


static int gx_toddr_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_prepare");
	return 0;
}


static int gx_toddr_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_trigger");
	return 0;
}


static int gx_toddr_dai_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_hw_params");
	return 0;
}


static const struct snd_soc_dai_ops gx_toddr_dai_ops = {
	.startup = gx_toddr_dai_startup,
	.shutdown = gx_toddr_dai_shutdown,
	.prepare = gx_toddr_dai_prepare,
	.trigger = gx_toddr_dai_trigger,
	.hw_params = gx_toddr_dai_hw_params,
};


static int gx_toddr_dai_probe(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_probe");
	return 0;
}


static int gx_toddr_dai_remove(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "gx_toddr_dai_remove");
	return 0;
}


static struct snd_soc_dai_driver gx_toddr_dai_driver[] = {
	{
		.name = "TODDR0",
		.id = 0,
		.probe = gx_toddr_dai_probe,
		.remove = gx_toddr_dai_remove,
		.ops = &gx_toddr_dai_ops,
		.capture = {
			.stream_name = "TODDR0 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	},
};


static int gx_toddr_platform_probe(struct platform_device *pdev)
{
	int ret;

	dev_dbg(&pdev->dev, "gx_toddr_platform_probe");

	ret = snd_soc_register_component(&pdev->dev, &gx_toddr_component_driver, gx_toddr_dai_driver, ARRAY_SIZE(gx_toddr_dai_driver));
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to register TODDR component: %d", ret);
		return ret;
	}

	return 0;
}


static int gx_toddr_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_toddr_platform_remove");

	snd_soc_unregister_component(&pdev->dev);

	return 0;
}


static const struct of_device_id gx_toddr_of_match[] = {
	{ .compatible = "amlogic,gx-toddr" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_toddr_of_match);


static struct platform_driver gx_toddr_platform_driver = {
	.driver = {
		.name = "gx-toddr",
		.of_match_table = gx_toddr_of_match,
	},
	.probe = gx_toddr_platform_probe,
	.remove = gx_toddr_platform_remove,
};
module_platform_driver(gx_toddr_platform_driver);


MODULE_DESCRIPTION("Amlogic GX TODDR audio driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
