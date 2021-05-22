// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>


static int gx_card_platform_probe(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_card_platform_probe");
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
