// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "gx-audio.h"
#include "gx-aiu.h"


#define GX_I2S_FIFO_BLOCK_SIZE 256


static struct snd_pcm_hardware gx_i2sfifo_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID,
	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates = SNDRV_PCM_RATE_8000_192000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = GX_I2S_FIFO_BLOCK_SIZE * 32 * 64,
	.period_bytes_min = GX_I2S_FIFO_BLOCK_SIZE,
	.period_bytes_max = GX_I2S_FIFO_BLOCK_SIZE * 32,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = GX_I2S_FIFO_BLOCK_SIZE,
};


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
	int ret;

	dev_dbg(component->dev, "gx_i2sfifo_component_open");

	ret = snd_soc_set_runtime_hwparams(substream, &gx_i2sfifo_pcm_hardware);
	if (ret)
	{
		dev_err(component->dev, "Failed to set hardware parameters: %d", ret);
		return ret;
	}

	ret = snd_pcm_hw_constraint_step(substream->runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, GX_I2S_FIFO_BLOCK_SIZE);
	if (ret)
	{
		return ret;
	}

	ret = snd_pcm_hw_constraint_step(substream->runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, GX_I2S_FIFO_BLOCK_SIZE);
	if (ret)
	{
		return ret;
	}

	return 0;
}


static int gx_i2sfifo_component_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	dev_dbg(component->dev, "gx_i2sfifo_component_close");
	return 0;
}


static int gx_i2sfifo_pcm_allocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = gx_i2sfifo_pcm_hardware.buffer_bytes_max;

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


static int gx_i2sfifo_component_pcm_construct(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	int ret;
	struct snd_card *card;

	dev_dbg(component->dev, "gx_i2sfifo_pcm_construct");

	card = rtd->card->snd_card;

	ret = dma_coerce_mask_and_coherent(card->dev, DMA_BIT_MASK(32));
	if (ret)
	{
		dev_err(card->dev, "Setting the DMA mask failed: %d", ret);
		return ret;
	}

	if (rtd->pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream)
	{
		ret = gx_i2sfifo_pcm_allocate_dma_buffer(rtd->pcm, SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
		{
			dev_err(card->dev, "Allocating the DMA buffer failed: %d", ret);
			return ret;
		}
	}

	return 0;
}


static void gx_i2sfifo_pcm_deallocate_dma_buffer(struct snd_pcm *pcm, int stream)
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


static void gx_i2sfifo_component_pcm_destruct(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	dev_dbg(component->dev, "gx_i2sfifo_pcm_destruct");
	gx_i2sfifo_pcm_deallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
}


snd_pcm_uframes_t gx_i2sfifo_component_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct gx_audio *gx_audio;
	unsigned int addr;

	gx_audio = dev_get_drvdata(component->dev->parent);

	regmap_read(gx_audio->aiu_regmap, AIU_MEM_I2S_RD_PTR, &addr);

	return bytes_to_frames(substream->runtime, addr - (unsigned int)substream->runtime->dma_addr);
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


static irqreturn_t gx_i2sfifo_handler(int irq, void *dev_id)
{
	struct snd_pcm_substream *playback = dev_id;

	snd_pcm_period_elapsed(playback);

	return IRQ_HANDLED;
}


static int gx_i2sfifo_dai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int ret;
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2sfifo_dai_startup");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	ret = request_irq(gx_audio->aiu_irq_i2s, gx_i2sfifo_handler, 0, dev_name(dai->dev), substream);
	if (ret)
	{
		dev_err(dai->dev, "Failed to get interrupt: %d", ret);
		return ret;
	}

	return 0;
}


static void gx_i2sfifo_dai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2sfifo_dai_shutdown");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	free_irq(gx_audio->aiu_irq_i2s, substream);
}


static int gx_i2sfifo_dai_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;

	dev_dbg(dai->dev, "gx_i2sfifo_dai_prepare");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_BUF_CNTL, AIU_MEM_I2S_BUF_CNTL_INIT, AIU_MEM_I2S_BUF_CNTL_INIT);
	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_BUF_CNTL, AIU_MEM_I2S_BUF_CNTL_INIT, 0);

	return 0;
}


static int gx_i2sfifo_dai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;
	unsigned int value;
	unsigned int enable;

	dev_dbg(dai->dev, "gx_i2sfifo_dai_trigger");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	switch (cmd)
	{
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		regmap_write(gx_audio->aiu_regmap, AIU_RST_SOFT, AIU_RST_SOFT_I2S_FAST);
		regmap_read(gx_audio->aiu_regmap, AIU_I2S_SYNC, &value);

		enable = (AIU_MEM_I2S_CONTROL_FILL_EN | AIU_MEM_I2S_CONTROL_EMPTY_EN);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		enable = 0;
		break;

	default:
		return -EINVAL;
	}

	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_CONTROL, AIU_MEM_I2S_CONTROL_FILL_EN | AIU_MEM_I2S_CONTROL_EMPTY_EN, enable);

	return 0;
}


static int gx_i2sfifo_dai_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct gx_audio *gx_audio;
	unsigned int value;

	dev_dbg(dai->dev, "gx_i2sfifo_dai_hw_params");

	gx_audio = dev_get_drvdata(dai->dev->parent);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	regmap_write(gx_audio->aiu_regmap, AIU_MEM_I2S_START_PTR, substream->runtime->dma_addr);
	regmap_write(gx_audio->aiu_regmap, AIU_MEM_I2S_RD_PTR, substream->runtime->dma_addr);
	regmap_write(gx_audio->aiu_regmap, AIU_MEM_I2S_END_PTR, substream->runtime->dma_addr + params_buffer_bytes(params) - GX_I2S_FIFO_BLOCK_SIZE);

	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_MASKS, AIU_MEM_I2S_MASKS_READ | AIU_MEM_I2S_MASKS_MEMORY, FIELD_PREP(AIU_MEM_I2S_MASKS_READ, 0xff) | FIELD_PREP(AIU_MEM_I2S_MASKS_MEMORY, 0xff));

	switch (params_physical_width(params))
	{
	case 16:
		value = AIU_MEM_I2S_CONTROL_MODE_16BIT;
		break;

	case 32:
		value = 0;
		break;

	default:
		dev_err(dai->dev, "Unsupported physical width: %u", params_physical_width(params));
		return -EINVAL;
	}
	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_CONTROL, AIU_MEM_I2S_CONTROL_MODE_16BIT, value);

	value = params_period_bytes(params) / GX_I2S_FIFO_BLOCK_SIZE;
	regmap_update_bits(gx_audio->aiu_regmap, AIU_MEM_I2S_MASKS, AIU_MEM_I2S_MASKS_IRQ_BLOCK, FIELD_PREP(AIU_MEM_I2S_MASKS_IRQ_BLOCK, value));

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
		dev_err(&pdev->dev, "Failed to register I2SFIFO component: %d", ret);
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
