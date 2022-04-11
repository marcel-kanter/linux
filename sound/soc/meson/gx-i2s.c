// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "gx-audio.h"
#include "gx-aiu.h"


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


static const unsigned int gx_i2s_dai_channels[] = {2, 8};


static const struct snd_pcm_hw_constraint_list gx_i2s_dai_channel_constraints = {
	.list = gx_i2s_dai_channels,
	.count = ARRAY_SIZE(gx_i2s_dai_channels),
	.mask = 0,
};


static int gx_i2s_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2s_dai_startup");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	ret = snd_pcm_hw_constraint_list(substream->runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS, &gx_i2s_dai_channel_constraints);
	if (ret)
	{
		dev_err(dai->dev, "Adding channel constraints failed: %u", ret);
		return ret;
	}

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
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2s_dai_prepare");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_CONTROL, AIU_MEM_I2S_CONTROL_INIT, AIU_MEM_I2S_CONTROL_INIT);
	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_CONTROL, AIU_MEM_I2S_CONTROL_INIT, 0);

	return 0;
}


static int gx_i2s_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;
	unsigned int hold;

	dev_dbg(dai->dev, "gx_i2s_dai_trigger");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	switch (cmd)
	{
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		hold = 0;
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		hold = AIU_I2S_MISC_HOLD_EN;
		break;

	default:
		return -EINVAL;
	}

	regmap_update_bits(gx_audio->aiu_regmap, AIU_I2S_MISC, AIU_I2S_MISC_HOLD_EN, hold);

	return 0;
}


static int gx_i2s_dai_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int ret;
	struct gx_audio *gx_audio;
	unsigned int value;
	unsigned int fs, bs;

	dev_dbg(dai->dev, "gx_i2s_dai_hw_params");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	ret = regmap_update_bits(gx_audio->aiu_regmap, AIU_CLK_CTRL, AIU_CLK_CTRL_I2S_DIV_EN, 0);
	if (ret)
	{
		dev_err(dai->dev, "Disable I2S divider failed: %d", ret);
		return ret;
	}

	regmap_write(gx_audio->aiu_regmap, AIU_RST_SOFT, AIU_RST_SOFT_I2S_FAST);
	regmap_read(gx_audio->aiu_regmap, AIU_I2S_SYNC, &value);

	value = AIU_I2S_SOURCE_DESC_MODE_SPLIT;
	switch (params_physical_width(params))
	{
	case 16:
		break;

	case 32:
		value |= (AIU_I2S_SOURCE_DESC_MODE_24BIT | AIU_I2S_SOURCE_DESC_MODE_32BIT);
		break;

	default:
		dev_err(dai->dev, "Unsupported physical width: %u", params_physical_width(params));
		return -EINVAL;
	}

	switch (params_channels(params))
	{
	case 2:
		break;
	case 8:
		value |= AIU_I2S_SOURCE_DESC_MODE_8CH;
		break;
	default:
		dev_err(dai->dev, "Unsupported channel number: %u", params_channels(params));
		return -EINVAL;
	}

	regmap_update_bits(gx_audio->aiu_regmap, AIU_I2S_SOURCE_DESC, AIU_I2S_SOURCE_DESC_MODE_8CH | AIU_I2S_SOURCE_DESC_MODE_24BIT | AIU_I2S_SOURCE_DESC_MODE_32BIT | AIU_I2S_SOURCE_DESC_MODE_SPLIT, value);

	fs = DIV_ROUND_CLOSEST(clk_get_rate(gx_audio->aiu_i2s_clocks[AIU_CLK_I2S_MCLK].clk), params_rate(params));
	if (fs % 64)
	{
		dev_err(dai->dev, "Frame clock divider error: %u", fs);
		return -EINVAL;
	}
	regmap_update_bits(gx_audio->aiu_regmap, AIU_CODEC_DAC_LRCLK_CTRL, AIU_CODEC_DAC_LRCLK_CTRL_DIV, FIELD_PREP(AIU_CODEC_DAC_LRCLK_CTRL_DIV, 64 - 1));

	bs = fs / 64;
	if (bs > 64)
	{
		dev_err(dai->dev, "Bit clock divider error: %u", bs);
		return -EINVAL;
	}
	regmap_update_bits(gx_audio->aiu_regmap, AIU_CLK_CTRL, AIU_CLK_CTRL_I2S_DIV, FIELD_PREP(AIU_CLK_CTRL_I2S_DIV, 0));
	regmap_update_bits(gx_audio->aiu_regmap, AIU_CLK_CTRL_MORE, AIU_CLK_CTRL_MORE_I2S_DIV, FIELD_PREP(AIU_CLK_CTRL_MORE_I2S_DIV, bs - 1));

	ret = regmap_update_bits(gx_audio->aiu_regmap, AIU_CLK_CTRL, AIU_CLK_CTRL_I2S_DIV_EN, AIU_CLK_CTRL_I2S_DIV_EN);
	if (ret)
	{
		dev_err(dai->dev, "Enable I2S divider failed: %d", ret);
		return ret;
	}

	return 0;
}


static int gx_i2s_dai_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2s_dai_hw_free");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	ret = regmap_update_bits(gx_audio->aiu_regmap, AIU_CLK_CTRL, AIU_CLK_CTRL_I2S_DIV_EN, 0);
	if (ret)
	{
		dev_err(dai->dev, "Disable divider failed: %u", ret);
		return ret;
	}

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
