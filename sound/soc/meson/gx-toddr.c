// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2022 - 2023 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "gx-audio.h"
#include "gx-audin.h"


#define GX_TODDR_FIFO_SIZE 512


static struct snd_pcm_hardware gx_toddr_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates = SNDRV_PCM_RATE_8000_48000,
	.channels_min = 1,
	.channels_max = 8,
	.buffer_bytes_max = GX_TODDR_FIFO_SIZE * 32 * 64,
	.period_bytes_min = GX_TODDR_FIFO_SIZE,
	.period_bytes_max = GX_TODDR_FIFO_SIZE * 32,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = GX_TODDR_FIFO_SIZE,
};


static const char *gx_toddr_mux_texts[] = {"SPDIF", "I2S", "PCM"};


static const struct soc_enum gx_toddr_mux_enum = SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(gx_toddr_mux_texts), gx_toddr_mux_texts);


static int gx_toddr_mux_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct snd_soc_dapm_context *dapm = snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_card *card = dapm->card;
	struct gx_audio *gx_audio;
	unsigned int value;

	dev_dbg(card->dev, "gx_toddr_mux_get");

	gx_audio = dev_get_drvdata(component->dev->parent);

	regmap_read(gx_audio->audin_regmap, AUDIN_FIFO0_CTRL0, &value);
	ucontrol->value.enumerated.item[0] = FIELD_GET(AUDIN_FIFO0_CTRL0_DIN_SEL, value);

	return 0;
}


static int gx_toddr_mux_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_dapm_kcontrol_component(kcontrol);
	struct snd_soc_dapm_context *dapm = snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_card *card = dapm->card;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct gx_audio *gx_audio;
	unsigned int mux, value;

	dev_dbg(card->dev, "gx_toddr_mux_put");

	gx_audio = dev_get_drvdata(component->dev->parent);

	mux = snd_soc_enum_item_to_val(e, ucontrol->value.enumerated.item[0]);

	regmap_read(gx_audio->audin_regmap, AUDIN_FIFO0_CTRL0, &value);

	if (FIELD_GET(AUDIN_FIFO0_CTRL0_DIN_SEL, value) != mux)
	{
		regmap_update_bits(gx_audio->audin_regmap, AUDIN_FIFO0_CTRL0, AUDIN_FIFO0_CTRL0_DIN_SEL, FIELD_PREP(AUDIN_FIFO0_CTRL0_DIN_SEL, mux));
		return snd_soc_dapm_mux_update_power(dapm, kcontrol, mux, e, NULL);
	}
	else
	{
		return 0;
	}
}


static const struct snd_kcontrol_new gx_toddr_controls[] = {
	SOC_DAPM_ENUM_EXT("TODDR0 Source", gx_toddr_mux_enum, gx_toddr_mux_get, gx_toddr_mux_put),
};


static const struct snd_soc_dapm_widget gx_toddr_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN("TODDR SPDIF", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("TODDR I2S", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("TODDR PCM", NULL, 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_MUX("TODDR0 Source", SND_SOC_NOPM, 0, 0, &gx_toddr_controls[0]),
};


static const struct snd_soc_dapm_route gx_toddr_dapm_routes[] = {
	{"TODDR0 Capture", NULL, "TODDR0 Source"},
	{"TODDR0 Source", "SPDIF", "TODDR SPDIF"},
	{"TODDR0 Source", "I2S", "TODDR I2S"},
	{"TODDR0 Source", "PCM", "TODDR PCM"},
};


static int gx_toddr_component_probe(struct snd_soc_component *component)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(component->dev, "gx_toddr_component_probe");

	gx_audio = dev_get_drvdata(component->dev->parent);

	ret = clk_prepare_enable(gx_audio->audin_clocks[AUDIN_CLK_AUDIN_CLK].clk);
	if (ret)
	{
		dev_err(component->dev, "Failed to enable peripheral clock");
		return ret;
	}

	return 0;
}


static void gx_toddr_component_remove(struct snd_soc_component *component)
{
	struct gx_audio *gx_audio;

	dev_dbg(component->dev, "gx_toddr_component_remove");

	gx_audio = dev_get_drvdata(component->dev->parent);

	clk_disable_unprepare(gx_audio->audin_clocks[AUDIN_CLK_AUDIN_CLK].clk);
}


static int gx_toddr_component_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	int ret;

	dev_dbg(component->dev, "gx_toddr_component_open");

	ret = snd_soc_set_runtime_hwparams(substream, &gx_toddr_pcm_hardware);
	if (ret)
	{
		dev_err(component->dev, "Failed to set hardware parameters: %d", ret);
		return ret;
	}

	ret = snd_pcm_hw_constraint_step(substream->runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, GX_TODDR_FIFO_SIZE);
	if (ret)
	{
		return ret;
	}

	ret = snd_pcm_hw_constraint_step(substream->runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, GX_TODDR_FIFO_SIZE);
	if (ret)
	{
		return ret;
	}

	return 0;
}


static int gx_toddr_component_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_toddr_component_close");
	return 0;
}


static int gx_toddr_pcm_allocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = gx_toddr_pcm_hardware.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_coherent(pcm->card->dev, size, &buf->addr, GFP_KERNEL);

	if (!buf->area)
	{
		return -ENOMEM;
	}

	buf->bytes = size;

	return 0;
}


static int gx_toddr_component_pcm_construct(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	int ret;
	struct snd_card *card;

	dev_dbg(component->dev, "gx_toddr_pcm_construct");

	card = rtd->card->snd_card;

	ret = dma_coerce_mask_and_coherent(card->dev, DMA_BIT_MASK(32));
	if (ret)
	{
		dev_err(card->dev, "Setting the DMA mask failed: %d", ret);
		return ret;
	}

	if (rtd->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream)
	{
		ret = gx_toddr_pcm_allocate_dma_buffer(rtd->pcm, SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
		{
			dev_err(card->dev, "Allocating the DMA buffer failed: %d", ret);
			return ret;
		}
	}

	return 0;
}


static void gx_toddr_pcm_deallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	substream = pcm->streams[stream].substream;
	if (!substream)
	{
		return;
	}

	buf = &substream->dma_buffer;
	if (!buf->area)
	{
		return;
	}

	dma_free_coherent(pcm->card->dev, buf->bytes, buf->area, buf->addr);
	buf->area = NULL;
}


static void gx_toddr_component_pcm_destruct(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	dev_dbg(component->dev, "gx_toddr_pcm_destruct");
	gx_toddr_pcm_deallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
}


snd_pcm_uframes_t gx_toddr_component_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	return 0;
}


static const struct snd_soc_component_driver gx_toddr_component_driver = {
	.name = "TODDR",
	.dapm_widgets = gx_toddr_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(gx_toddr_dapm_widgets),
	.dapm_routes = gx_toddr_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(gx_toddr_dapm_routes),
	.probe = gx_toddr_component_probe,
	.remove = gx_toddr_component_remove,
	.open = gx_toddr_component_open,
	.close = gx_toddr_component_close,
	.pcm_construct = gx_toddr_component_pcm_construct,
	.pcm_destruct = gx_toddr_component_pcm_destruct,
	.pointer = gx_toddr_component_pointer,
};


static irqreturn_t gx_audin_handler(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}


static int gx_toddr_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_toddr_dai_startup");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	// Disable address match interrupt until hardware is configured
	regmap_update_bits(gx_audio->audin_regmap, AUDIN_INT_CTRL, AUDIN_INT_CTRL_FIFO0_ADDRESS, 0);

	ret = request_irq(gx_audio->audin_irq_audin, gx_audin_handler, 0, dev_name(dai->dev), substream);
	if (ret)
	{
		dev_err(dai->dev, "Failed to get interrupt: %d", ret);
		return ret;
	}

	return 0;
}


static void gx_toddr_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_toddr_dai_shutdown");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	// Disable address match interrupt
	regmap_update_bits(gx_audio->audin_regmap, AUDIN_INT_CTRL, AUDIN_INT_CTRL_FIFO0_ADDRESS, 0);

	free_irq(gx_audio->audin_irq_audin, substream);
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
