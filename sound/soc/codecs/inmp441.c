// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2022 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>

#include <sound/soc.h>


struct inmp441
{
	struct gpio_desc *enable_gpio;
};


static int inmp441_aif_event(struct snd_soc_dapm_widget *widget, struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component;
	struct inmp441 *inmp441;

	component = snd_soc_dapm_to_component(widget->dapm);

	dev_dbg(component->dev, "inmp441_aif_event");

	inmp441 = snd_soc_component_get_drvdata(component);

	switch (event)
	{
	case SND_SOC_DAPM_POST_PMU:
		if (inmp441->enable_gpio)
			gpiod_set_value_cansleep(inmp441->enable_gpio, 1);
		break;

	case SND_SOC_DAPM_POST_PMD:
		if (inmp441->enable_gpio)
			gpiod_set_value_cansleep(inmp441->enable_gpio, 0);
		break;
	}

	return 0;
}


static const struct snd_soc_dapm_widget inmp441_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC"),
	SND_SOC_DAPM_AIF_OUT_E("AIF", "Capture", 0, SND_SOC_NOPM, 0, 0, inmp441_aif_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
};


static const struct snd_soc_dapm_route inmp441_dapm_routes[] = {
	{ "AIF", NULL, "MIC" },
};


static const struct snd_soc_component_driver inmp441_component_driver = {
	.idle_bias_on = 1,
	.dapm_widgets = inmp441_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(inmp441_dapm_widgets),
	.dapm_routes = inmp441_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(inmp441_dapm_routes),
	.use_pmdown_time = 1,
	.endianness = 1,
};


static int inmp441_dai_ops_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	dev_dbg(dai->component->dev, "inmp441_dai_ops_trigger");

	return 0;
}


static const struct snd_soc_dai_ops inmp441_dai_ops = {
	.trigger = inmp441_dai_ops_trigger,
};


static struct snd_soc_dai_driver inmp441_dai_driver[] = {
	{
		.name = "INMP441",
		.ops = &inmp441_dai_ops,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = SNDRV_PCM_RATE_CONTINUOUS,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	},
};


static int inmp441_platform_probe(struct platform_device *pdev)
{
	struct inmp441 *inmp441;

	dev_dbg(&pdev->dev, "inmp441_platform_probe");

	inmp441 = devm_kzalloc(&pdev->dev, sizeof(*inmp441), GFP_KERNEL);
	if (!inmp441)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for driver data");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, inmp441);

	inmp441->enable_gpio = devm_gpiod_get_optional(&pdev->dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(inmp441->enable_gpio))
	{
		dev_err(&pdev->dev, "Error while getting enable gpio: %ld", PTR_ERR(inmp441->enable_gpio));
		return PTR_ERR(inmp441->enable_gpio);
	}

	return devm_snd_soc_register_component(&pdev->dev, &inmp441_component_driver, inmp441_dai_driver, ARRAY_SIZE(inmp441_dai_driver));
}


static const struct of_device_id inmp441_of_match[] = {
	{ .compatible = "invensense,inmp441" },
	{ }
};
MODULE_DEVICE_TABLE(of, inmp441_of_match);


static struct platform_driver inmp441_platform_driver = {
	.driver = {
		.name = "inmp441",
		.of_match_table = inmp441_of_match,
	},
	.probe = inmp441_platform_probe,
};
module_platform_driver(inmp441_platform_driver);


MODULE_DESCRIPTION("InvenSense INMP441 microphone driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");

