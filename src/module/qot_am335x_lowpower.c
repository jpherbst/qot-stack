/*
 * @file qot_bbb.c
 * @brief Linux 4.1.6 kernel module for managing timers on A BeagleBone Black
 * @author Andrew Symington
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

IMPORTANT : To use this driver you will need to add the following snippet to your
            default device tree, or to an overlay (if supported). This will cause
            the module to be loaded on boot, and make use of the QoT stack.

qot_am335x_lowpower {
	compatible = "qot_am335x_lowpower";
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&qot_am335x_lowpower_pins>;
};

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>

struct qot_am335x_lowpower_data {
	int state;
};

static const struct of_device_id qot_am335x_lowpower_dt_ids[] = {
	{ .compatible = "qot_am335x_lowpower", },
  	{ }
};

MODULE_DEVICE_TABLE(of, qot_am335x_lowpower_dt_ids);

static int qot_am335x_lowpower_probe(struct platform_device *pdev)
{
	const struct of_device_id *match = 
		 of_match_device(qot_am335x_lowpower_dt_ids, &pdev->dev);
	if (match)
	{
		pdev->dev.platform_data = 
			devm_kzalloc(&pdev->dev, sizeof(struct qot_am335x_lowpower_data), GFP_KERNEL);
		if (!pdev->dev.platform_data)
			return -ENODEV;
	}
	else
	{
		pr_err("of_match_device failed\n");
		return -ENODEV;
	}
	return 0;
}

static int qot_am335x_lowpower_remove(struct platform_device *pdev)
{
	if (pdev->dev.platform_data)
	{
		devm_kfree(&pdev->dev, pdev->dev.platform_data);
		pdev->dev.platform_data = NULL;
	}
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver qot_am335x_lowpower_driver = {
  .probe    = qot_am335x_lowpower_probe,
  .remove   = qot_am335x_lowpower_remove,
	.driver   = {
		.name = "qot_am335x_lowpower",			
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(qot_am335x_lowpower_dt_ids),
	},
};

module_platform_driver(qot_am335x_lowpower_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hani Esmaeelzadeh");
MODULE_DESCRIPTION("QoT Driver for Linux 4.1.6");
MODULE_VERSION("0.4.0");
