// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Driver for the PCM5102A codec

Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/soc.h>


struct pcm5102a
{
	struct gpio_desc *mute_gpio;
};


static const struct snd_soc_dapm_widget pcm5102a_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "Playback", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_OUTPUT("OUTL"),
	SND_SOC_DAPM_OUTPUT("OUTR"),
};


static const struct snd_soc_dapm_route pcm5102a_dapm_routes[] = {
	{ "OUTL", NULL, "DAC" },
	{ "OUTR", NULL, "DAC" },
};


static struct snd_soc_component_driver pcm5102a_component_driver = {
	.idle_bias_on = 1,
	.dapm_widgets = pcm5102a_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(pcm5102a_dapm_widgets),
	.dapm_routes = pcm5102a_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(pcm5102a_dapm_routes),
	.use_pmdown_time = 1,
	.endianness = 1,
	.non_legacy_dai_naming = 1,
};


static int pcm5102a_dai_ops_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component;
	struct pcm5102a *pcm5102a;

	component = dai->component;
	pcm5102a = snd_soc_component_get_drvdata(dai->component);

	dev_dbg(component->dev, "pcm5102a_dai_ops_trigger");

	if (!pcm5102a->mute_gpio)
	{
		return 0;
	}

	switch (cmd)
	{
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		gpiod_set_value(pcm5102a->mute_gpio, 1);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		gpiod_set_value(pcm5102a->mute_gpio, 0);
		break;
	}

	return 0;
}


static const struct snd_soc_dai_ops pcm5102a_dai_ops = {
	.trigger = pcm5102a_dai_ops_trigger,
};


static struct snd_soc_dai_driver pcm5102a_dai_driver = {
	.name = "pcm5102a",
	.ops = &pcm5102a_dai_ops,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	},
};


static int pcm5102a_platform_probe(struct platform_device *pdev)
{
	struct pcm5102a *pcm5102a;

	pcm5102a = devm_kzalloc(&pdev->dev, sizeof(*pcm5102a), GFP_KERNEL);
	if (!pcm5102a)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for driver data");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, pcm5102a);

	pcm5102a->mute_gpio = devm_gpiod_get_optional(&pdev->dev, "mute", GPIOD_OUT_LOW);
	if (IS_ERR(pcm5102a->mute_gpio))
	{
		dev_err(&pdev->dev, "Error while getting mute gpio: %ld", PTR_ERR(pcm5102a->mute_gpio));
		return PTR_ERR(pcm5102a->mute_gpio);
	}

	return devm_snd_soc_register_component(&pdev->dev, &pcm5102a_component_driver, &pcm5102a_dai_driver, 1);
}


static const struct of_device_id pcm5102a_of_match[] = {
	{ .compatible = "ti,pcm5102a" },
	{ }
};
MODULE_DEVICE_TABLE(of, pcm5102a_of_match);


static struct platform_driver pcm5102a_platform_driver = {
	.driver = {
		.name = "pcm5102a",
		.of_match_table = pcm5102a_of_match,
	},
	.probe = pcm5102a_platform_probe,
};
module_platform_driver(pcm5102a_platform_driver);


MODULE_DESCRIPTION("PCM5102A codec driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
