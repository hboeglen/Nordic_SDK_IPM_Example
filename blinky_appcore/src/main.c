/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/ipm.h>
#include <hal/nrf_spu.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

static const struct device *ipm_tx_handle;
static const struct device *ipm_rx_handle;
static struct sensor_value val __attribute__((__section__(".shared_ram_sensor")));

void ipm_tx_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	printk("TX ID %d\n", id);
}

void ipm_rx_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	printk("RX ID %d\n", id);	
	printk("Temp %d.%d\n", val.val1, val.val2);
}

void spu_setup(void)
{
	/* Since we're in secure mode, we need to explicitly
	 * allow the net core to access the secure mapped RAM. 
	 */
	nrf_spu_extdomain_set(NRF_SPU, 0, true, false);
}

int main(void)
{
	//int ret;
	spu_setup();
	printk("App Core\n");
	ipm_tx_handle = device_get_binding("IPM_0");
	if (!ipm_tx_handle) {
		printk("Could not get TX IPM device handle");
		return -1;
	}
	else
		printk("Got TX IPM device handle\n");

	ipm_rx_handle = device_get_binding("IPM_1");
	if (!ipm_rx_handle) {
		printk("Could not get RX IPM device handle");
		return -1;
	}
	else
		printk("Got RX IPM device handle\n");	

	ipm_register_callback(ipm_tx_handle, ipm_tx_callback, NULL);
	ipm_register_callback(ipm_rx_handle, ipm_rx_callback, NULL);

	ipm_set_enabled(ipm_tx_handle, 1);
	ipm_set_enabled(ipm_rx_handle, 1);

	while (1) {
		k_msleep(SLEEP_TIME_MS);
		/* Trigger netcore to write into our shared memory */
		uint32_t ret = ipm_send(ipm_tx_handle, 1, 0, NULL, 0);
		if (ret) {
			printk("ret value = %d\n", ret);
		}		
	}
}
