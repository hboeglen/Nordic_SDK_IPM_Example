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

/* Empty symbol from linker script. 
 * Its only purpose is to give us the shared RAM address space.
 */
extern uint32_t __shared_ram_start;
#define SHARED_RAM_ADDRESS &__shared_ram_start

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

static struct sensor_value val;
static const struct device *ipm_tx_handle;
static const struct device *ipm_rx_handle;
struct sensor_value *p_val;

static bool data_requested = true;

void ipm_tx_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	printk("TX ID %d\n", id);
}

void ipm_rx_callback(const struct device *dev, void *context,
		       uint32_t id, volatile void *data)
{
	printk("RX ID %d\n", id);

	p_val = (struct sensor_value *)SHARED_RAM_ADDRESS;
	p_val->val1 = val.val1;
	p_val->val2 = val.val2;
	//uint32_t ret = 
	ipm_send(ipm_tx_handle, 1, 0, NULL, 0);
}

int main(void)
{
	const struct device *temp_dev = DEVICE_DT_GET_ANY(nordic_nrf_temp);

	printk("Network core temp sample. Fetches from shared RAM address: %p\n", SHARED_RAM_ADDRESS);
	int ret;
	//temp_dev = device_get_binding("TEMP_0");
	if (temp_dev == NULL || !device_is_ready(temp_dev)) {
		printk("no temperature device; using simulated data\n");
		temp_dev = NULL;
	} else {
		printk("temp device is %p, name is %s\n", temp_dev,
		       temp_dev->name);
	}
	/*if (!temp_dev) {
		printk("Didn't find the temp driver!\n");
		return -1;
	}*/

	/* IPM setup */
	ipm_rx_handle = device_get_binding("IPM_0");
	if (!ipm_rx_handle) {
		printk("Could not get TX IPM device handle");
		return -1;
	}
	ipm_tx_handle = device_get_binding("IPM_1");
	if (!ipm_tx_handle) {
		printk("Could not get TX IPM device handle");
		return -1;
	}	

	ipm_register_callback(ipm_tx_handle, ipm_tx_callback, NULL);
	ipm_register_callback(ipm_rx_handle, ipm_rx_callback, NULL);
	ipm_set_enabled(ipm_tx_handle, 1);
	ipm_set_enabled(ipm_rx_handle, 1);
	
	while (1) {

		ret = sensor_sample_fetch(temp_dev);

		if (ret) {
			printk("Failed to sample temp sensor!\n");
			return -1;
		}

		ret = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);

		if (ret) {
			printk("failed to get temp sensor data!\n");
			return -1;
		}
		printk("Temp %d.%d\n", val.val1, val.val2);
		k_msleep(SLEEP_TIME_MS);
	}
}
