/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#include <stdio.h>

#ifdef CONFIG_VE_SIM
#include <simulator.h>
#endif


uint32_t count = 0;

#ifdef CONFIG_GPIO
static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(
		DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_callback;

static void button_isr_callback(const struct device *port,
				struct gpio_callback *cb,
				uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	count = 0;
}
#endif

static void lv_btn_click_callback(lv_event_t *e)
{
	ARG_UNUSED(e);

	count++;
}

void cb_for_btns(void)
{
	count++;
}

#ifdef CONFIG_LV_Z_ENCODER_INPUT
static const struct device *lvgl_encoder =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_encoder_input));
#endif /* CONFIG_LV_Z_ENCODER_INPUT */

#ifdef CONFIG_LV_Z_KEYPAD_INPUT
static const struct device *lvgl_keypad =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_keypad_input));
#endif /* CONFIG_LV_Z_KEYPAD_INPUT */

int main(void)
{
	const struct device *display_dev;
	lv_obj_t *hello_world_label;
	char count_str[20] = {0};

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		return 0;
	}
  

	#ifdef CONFIG_GPIO
	if (gpio_is_ready_dt(&button_gpio)) {
		int err;

		err = gpio_pin_configure_dt(&button_gpio, GPIO_INPUT);
		if (err) {			
			return 0;
		}

		gpio_init_callback(&button_callback, button_isr_callback,
				   BIT(button_gpio.pin));

		err = gpio_add_callback(button_gpio.port, &button_callback);
		if (err) {			
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(&button_gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			return 0;
		}
	}
#endif /* CONFIG_GPIO */

	printf("%d", IS_ENABLED(CONFIG_LV_Z_POINTER_KSCAN)); 
	printf("%d \n", IS_ENABLED(CONFIG_LV_Z_POINTER_INPUT));
	#ifdef CONFIG_VE_SIM
	create_sim();
	#endif /* CONFIG_VT_SIM */

	lv_obj_t *screen = lv_obj_create(lv_scr_act());	
	lv_obj_set_size(screen, 550, 350);
	lv_obj_set_pos(screen, 75, 75);
	lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

	
	hello_world_label = lv_label_create(screen);	
	//hello_world_label = lv_label_create(lv_scr_act());	

	lv_label_set_text(hello_world_label, "Hello world!");
	lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *hello_world_button;

		hello_world_button = lv_btn_create(screen);
		lv_obj_align(hello_world_button, LV_ALIGN_CENTER, 0, -50);
		lv_obj_add_event_cb(hello_world_button, lv_btn_click_callback, LV_EVENT_CLICKED,
				    NULL);

	printf("Hello World! %s\n", CONFIG_BOARD);
	lv_task_handler();
	display_blanking_off(display_dev);
	
	while (1) {	
		sprintf(count_str, "Hello World! %d", count);
		lv_label_set_text(hello_world_label, count_str);	
		lv_task_handler();	
		if (BTN_FLAGS_TEST)
		{
			count++;		
			BTN_FLAGS_TEST = 0;	
		}		
		k_sleep(K_MSEC(10));
	}

	return 0;
}
