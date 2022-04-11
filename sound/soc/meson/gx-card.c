// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <sound/soc.h>


static int gx_card_parse_dai(struct snd_soc_card *card, struct device_node *node, struct snd_soc_dai_link_component *component)
{
	int ret;
	struct of_phandle_args args;
	const char *name;

	ret = of_parse_phandle_with_args(node, "sound-dai", "#sound-dai-cells", 0, &args);
	if (ret)
	{
		dev_err(card->dev, "Failed to parse sound-dai property: %d", ret);
		return ret;
	}
	component->of_node = args.np;

	ret = snd_soc_get_dai_name(&args, &name);
	if (ret)
	{
		if (ret == -EPROBE_DEFER)
		{
			dev_dbg(card->dev, "Failed to get dai name: %d", ret);
		}
		else
		{
			dev_err(card->dev, "Failed to get dai name: %d", ret);
		}
		return ret;
	}

	component->dai_name = devm_kstrdup(card->dev, name, GFP_KERNEL);
	if (!component->dai_name)
	{
		dev_err(card->dev, "Failed to allocate memory for the dai name");
		return -ENOMEM;
	}

	return 0;
}


static int gx_card_parse_link(struct snd_soc_card *card, struct device_node *node, struct snd_soc_dai_link *link)
{
	int ret;
	unsigned int num_cpus;
	unsigned int num_codecs;
	struct device_node *child;
	struct snd_soc_dai_link_component *components;
	unsigned int g;
	unsigned int h;

	num_cpus = 0;
	num_codecs = 0;
	for_each_child_of_node(node, child)
	{
		if (strncmp(child->name, "cpu", 3) == 0)
		{
			num_cpus++;
		}
		if (strncmp(child->name, "codec", 5) == 0)
		{
			num_codecs++;
		}
	}

	if ((num_cpus == 0) && (num_codecs == 0))
	{
		dev_err(card->dev, "No components for link %s", node->full_name);
		return -EINVAL;
	}

	// Keep space for the dummy components
	if (num_cpus == 0)
	{
		link->num_cpus = 1;
	}
	else
	{
		link->num_cpus = num_cpus;
	}
	if (num_codecs == 0)
	{
		link->num_codecs = 1;
	}
	else
	{
		link->num_codecs = num_codecs;
	}

	components = devm_kcalloc(card->dev, link->num_cpus + link->num_codecs, sizeof(*components), GFP_KERNEL);
	if (!components)
	{
		dev_err(card->dev, "Failed to allocate memory for the link components");
		return -ENOMEM;
	}

	g = 0;
	h = link->num_cpus;
	link->cpus = &components[g];
	link->codecs = &components[h];

	for_each_child_of_node(node, child)
	{
		if (strncmp(child->name, "cpu", 3) == 0)
		{
			ret = gx_card_parse_dai(card, child, &components[g]);
			if (ret)
			{
				goto error;
			}
			g++;
		}
		if (strncmp(child->name, "codec", 5) == 0)
		{
			ret = gx_card_parse_dai(card, child, &components[h]);
			if (ret)
			{
				goto error;
			}
			h++;
		}
	}

	if (num_cpus == 0)
	{
		link->cpus->name = "snd-soc-dummy";
		link->cpus->dai_name = "snd-soc-dummy-dai";

		link->no_pcm = 1;
	}

	if (num_codecs == 0)
	{
		link->codecs->name = "snd-soc-dummy";
		link->codecs->dai_name = "snd-soc-dummy-dai";

		link->dynamic = 1;
	}
	else
	{
		link->no_pcm = 1;
		link->ignore_pmdown_time = 1;
	}

	snd_soc_dai_link_set_capabilities(link);

	ret = of_property_read_string(node, "link-name", &link->name);
	if (ret)
	{
		dev_err(card->dev, "Failed to get link name: %d", ret);
		goto error;
	}
	link->stream_name = link->name;

error:
	of_node_put(child);
	return ret;
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

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret)
	{
		dev_err(&pdev->dev, "Failed to register card: %d", ret);
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
