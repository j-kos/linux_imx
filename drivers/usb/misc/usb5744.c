// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for the Microchip USB5744 4-port hub.
 *
 * Copyright (c) 2021 Xilinx, Inc.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>

static int usb5744_init_hw(struct device *dev)
{
	struct gpio_desc *reset_gpio;

	reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(reset_gpio)) {
		dev_err(dev, "Failed to bind reset gpio");
		return -ENODEV;
	}

	if (reset_gpio) {
		/* Toggle RESET_N to reset the hub. */
		gpiod_set_value(reset_gpio, 0);
		usleep_range(5, 20); /* trstia */
		gpiod_set_value(reset_gpio, 1);
		usleep_range(5000, 10000); /* tcsh */
	}

	return 0;
}

static int usb5744_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	int ret;

	/* Trigger gpio reset to the hub. */
	ret = usb5744_init_hw(dev);
	if (ret)
		return ret;

	/* Send SMBus command to boot hub. */
	ret = i2c_smbus_write_word_data(client, 0xAA, swab16(0x5600));
	if (ret < 0)
		dev_err(dev, "Sending boot command failed");

	return ret;
}

static int usb5744_platform_probe(struct platform_device *pdev)
{
	/* Trigger gpio reset to the hub. */
	return usb5744_init_hw(&pdev->dev);
}

static const struct i2c_device_id usb5744_id[] = {
	{ "usb5744", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, usb5744_id);

static struct i2c_driver usb5744_i2c_driver = {
	.driver = {
		.name = "usb5744",
	},
	.probe = usb5744_i2c_probe,
	.id_table = usb5744_id,
};

static const struct of_device_id usb5744_platform_id[] = {
	{ .compatible = "microchip,usb5744", },
	{ }
};

static struct platform_driver usb5744_platform_driver = {
	.driver = {
		.name = "microchip,usb5744",
		.of_match_table = usb5744_platform_id,
	},
	.probe = usb5744_platform_probe,
};

static int __init usb5744_init(void)
{
	int err;

	err = i2c_add_driver(&usb5744_i2c_driver);
	if (err != 0)
		pr_err("usb5744: Failed to register I2C driver: %d\n", err);

	err = platform_driver_register(&usb5744_platform_driver);
	if (err != 0)
		pr_err("usb5744: Failed to register platform driver: %d\n",
		       err);
	return 0;
}
module_init(usb5744_init);

static void __exit usb5744_exit(void)
{
	platform_driver_unregister(&usb5744_platform_driver);
	i2c_del_driver(&usb5744_i2c_driver);
}
module_exit(usb5744_exit);

MODULE_AUTHOR("Piyush Mehta <piyush.mehta@xilinx.com>");
MODULE_AUTHOR("Michal Simek <michal.simek@xilinx.com>");
MODULE_DESCRIPTION("USB5744 Hub");
MODULE_LICENSE("GPL v2");
