/*
 * @file dw1000.c
 * @brief Linux driver for the DecaWave DW1000/DWM1000 radio
 * @author Andrew Symington
 *
 * Copyright (c) Regents of the University of California, 2015.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Sufficient to develop a device-tree based platform driver */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>

#define MODULE_NAME "dw1000"

static const struct of_device_id dw1000_dt_ids[] = {
	{ .compatible = "dw1000", },
	{ }
};

MODULE_DEVICE_TABLE(of, dw1000_dt_ids);

static int dw1000_probe(struct platform_device *pdev) {
	return 0;
}

static int dw1000_remove(struct platform_device *pdev) {
	return 0;
}

static struct platform_driver dw1000_driver = {
	.probe    = dw1000_probe,
	.remove   = dw1000_remove,
	.driver   = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dw1000_dt_ids),
	},
};

module_platform_driver(dw1000_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington");
MODULE_DESCRIPTION("Linux driver for DecaWave DW1000 radio");
MODULE_VERSION("0.0.1");