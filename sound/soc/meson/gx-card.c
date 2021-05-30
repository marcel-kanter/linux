// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/soc.h>


static int gx_card_parse_link(struct snd_soc_card *card, struct device_node *node, struct snd_soc_dai_link *link)
{
	return 0;
}


static int gx_card_parse_links(struct snd_soc_card *card)
{
	int ret;
	int num_links;
	struct device_node *node;
	struct device_node *child;
	struct snd_soc_dai_link *link;

	node = card->dev->of_node;

	num_links = of_get_child_count(node);
	if (num_links == 0)
	{
		dev_err(card->dev, "No links defined");
		return -EINVAL;
	}

	card->num_links = num_links;

	card->dai_link = devm_kcalloc(card->dev, card->num_links, sizeof(*link), GFP_KERNEL);
	if (!card->dai_link)
	{
		dev_err(card->dev, "Failed to allocate memory for the links");
		return -ENOMEM;
	}

	link = card->dai_link;
	for_each_child_of_node(node, child)
	{
		ret = gx_card_parse_link(card, child, link);
		if (ret)
		{
			goto error;
		}

		link++;
	}

error:
	of_node_put(child);
	return ret;
}


static int gx_card_platform_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_soc_card *card;

	dev_dbg(&pdev->dev, "gx_card_platform_probe");

	card = devm_kzalloc(&pdev->dev, sizeof(*card), GFP_KERNEL);
	if (!card)
	{
		dev_err(&pdev->dev, "Failed to allocate memory for sound card structure");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, card);

	card->owner = THIS_MODULE;
	card->dev = &pdev->dev;

	ret = snd_soc_of_parse_card_name(card, "model");
	if (ret)
	{
		return ret;
	}

	if (of_property_read_bool(card->dev->of_node, "audio-widgets"))
	{
		ret = snd_soc_of_parse_audio_simple_widgets(card, "audio-widgets");
		if (ret)
		{
			return ret;
		}
	}

	if (of_property_read_bool(card->dev->of_node, "audio-routing"))
	{
		ret = snd_soc_of_parse_audio_routing(card, "audio-routing");
		if (ret)
		{
			return ret;
		}
	}

	ret = gx_card_parse_links(card);
	if (ret)
	{
		return ret;
	}

	return 0;
}


static int gx_card_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_card_platform_remove");
	return 0;
}


static const struct of_device_id gx_card_of_match[] = {
	{ .compatible = "amlogic,gx-sound-card" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_card_of_match);


static struct platform_driver gx_card_platform_driver = {
	.driver = {
		.name = "gx-sound-card",
		.of_match_table = gx_card_of_match,
	},
	.probe = gx_card_platform_probe,
	.remove = gx_card_platform_remove,
};
module_platform_driver(gx_card_platform_driver);


MODULE_DESCRIPTION("Amlogic GX sound card driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
