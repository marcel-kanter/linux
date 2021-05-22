// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
Copyright (c) 2021 Marcel Kanter <marcel.kanter@googlemail.com>
*/
#include <linux/module.h>
#include <linux/of_device.h>


static int gx_audio_platform_probe(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_audio_platform_probe");
	return 0;
}


static int gx_audio_platform_remove(struct platform_device *pdev)
{
	dev_dbg(&pdev->dev, "gx_audio_platform_remove");
	return 0;
}


static const struct of_device_id gx_audio_of_match[] = {
	{ .compatible = "amlogic,gx-audio" },
	{ }
};
MODULE_DEVICE_TABLE(of, gx_audio_of_match);


static struct platform_driver gx_audio_platform_driver = {
	.driver = {
		.name = "gx-audio",
		.of_match_table = gx_audio_of_match,
	},
	.probe = gx_audio_platform_probe,
	.remove = gx_audio_platform_remove,
};
module_platform_driver(gx_audio_platform_driver);


MODULE_DESCRIPTION("Amlogic GX audio driver");
MODULE_AUTHOR("Marcel Kanter <marcel.kanter@googlemail.com>");
MODULE_LICENSE("Dual MIT/GPL");
