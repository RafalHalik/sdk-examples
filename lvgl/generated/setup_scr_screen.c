/*
* Copyright 2024 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"



void setup_scr_screen(lv_ui *ui)
{
	//Write codes screen
	ui->screen = lv_obj_create(NULL);
	lv_obj_set_size(ui->screen, 720, 1280);
	lv_obj_set_pos(ui->screen, 0, 0);
	lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_border_width(ui->screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_img_1
	lv_obj_set_style_bg_opa(ui->screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_src(ui->screen, &_BACKGROUND_720x1280, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_opa(ui->screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_recolor_opa(ui->screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	// lv_obj_set_style_img_recolor_opa(ui->screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_img_opa(ui->screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_radius(ui->screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_clip_corner(ui->screen_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_meter_1
	ui->screen_meter_1 = lv_meter_create(ui->screen);
	// add scale ui->screen_meter_1_scale_0
	ui->screen_meter_1_scale_0 = lv_meter_add_scale(ui->screen_meter_1);
	lv_meter_set_scale_ticks(ui->screen_meter_1, ui->screen_meter_1_scale_0, 21, 0, 10, lv_color_hex(0xffffff));
	lv_meter_set_scale_major_ticks(ui->screen_meter_1, ui->screen_meter_1_scale_0, 4, 0, 18, lv_color_hex(0xffffff), 200);
	lv_meter_set_scale_range(ui->screen_meter_1, ui->screen_meter_1_scale_0, 0, 50000, 262, 229);

	// add needle line for ui->screen_meter_1_scale_0.
	ui->screen_meter_1_scale_0_ndline_0 = lv_meter_add_needle_line(ui->screen_meter_1, ui->screen_meter_1_scale_0, 5, lv_color_hex(0x57f4ff), -30);
	lv_meter_set_indicator_value(ui->screen_meter_1, ui->screen_meter_1_scale_0_ndline_0, 0);
	lv_obj_set_pos(ui->screen_meter_1, 121, 390);
	lv_obj_set_size(ui->screen_meter_1, 491, 491);

	//Write style for screen_meter_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_meter_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_meter_1, 533, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_meter_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_meter_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_meter_1, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
	lv_obj_set_style_text_color(ui->screen_meter_1, lv_color_hex(0xff0000), LV_PART_TICKS|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_meter_1, &lv_font_montserratMedium_10, LV_PART_TICKS|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->screen_meter_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

	//Write style for screen_meter_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_meter_1, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes screen_img_2
	// ui->screen_img_2 = lv_img_create(ui->screen);
	// lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_CLICKABLE);
	// lv_img_set_src(ui->screen_img_2, &_centreBit_alpha_196x195);
	// lv_img_set_pivot(ui->screen_img_2, 50,50);
	// lv_img_set_angle(ui->screen_img_2, 0);
	// lv_obj_set_pos(ui->screen_img_2, 540, 256);
	// lv_obj_set_size(ui->screen_img_2, 196, 195);

	// //Write style for screen_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	// lv_obj_set_style_img_recolor_opa(ui->screen_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_img_opa(ui->screen_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_radius(ui->screen_img_2, 355, LV_PART_MAIN|LV_STATE_DEFAULT);
	// lv_obj_set_style_clip_corner(ui->screen_img_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_bar_1
	ui->screen_bar_1 = lv_bar_create(ui->screen);
	lv_obj_set_style_anim_time(ui->screen_bar_1, 1000, 0);
	lv_bar_set_mode(ui->screen_bar_1, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->screen_bar_1, 0, 1000);
	lv_bar_set_value(ui->screen_bar_1, 50, LV_ANIM_OFF);
	lv_obj_set_pos(ui->screen_bar_1, 232, 190);
	lv_obj_set_size(ui->screen_bar_1, 207, 27);

	//Write style for screen_bar_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_bar_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_bar_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_bar_1, lv_color_hex(0x02FFFF), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_bar_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_1, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes screen_bar_2
	ui->screen_bar_2 = lv_bar_create(ui->screen);
	lv_obj_set_style_anim_time(ui->screen_bar_2, 1000, 0);
	lv_bar_set_mode(ui->screen_bar_2, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->screen_bar_2, 0, 1000);
	lv_bar_set_value(ui->screen_bar_2, 50, LV_ANIM_OFF);
	lv_obj_set_pos(ui->screen_bar_2, 232, 79);
	lv_obj_set_size(ui->screen_bar_2, 209, 27);

	//Write style for screen_bar_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_2, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_bar_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_bar_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_2, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_bar_2, lv_color_hex(0xFF8901), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_bar_2, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_2, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes screen_bar_3
	ui->screen_bar_3 = lv_bar_create(ui->screen);
	lv_obj_set_style_anim_time(ui->screen_bar_3, 1000, 0);
	lv_bar_set_mode(ui->screen_bar_3, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->screen_bar_3, 0, 1000);
	lv_bar_set_value(ui->screen_bar_3, 50, LV_ANIM_OFF);
	lv_obj_set_pos(ui->screen_bar_3, 232, 1215);
	lv_obj_set_size(ui->screen_bar_3, 207, 27);

	//Write style for screen_bar_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_3, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_bar_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_bar_3, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_3, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_bar_3, lv_color_hex(0x02FFFF), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_bar_3, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_3, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes screen_bar_4
	ui->screen_bar_4 = lv_bar_create(ui->screen);
	lv_obj_set_style_anim_time(ui->screen_bar_4, 1000, 0);
	lv_bar_set_mode(ui->screen_bar_4, LV_BAR_MODE_NORMAL);
	lv_bar_set_range(ui->screen_bar_4, 0, 1000);
	lv_bar_set_value(ui->screen_bar_4, 50, LV_ANIM_OFF);
	lv_obj_set_pos(ui->screen_bar_4, 232, 1107);
	lv_obj_set_size(ui->screen_bar_4, 207, 28);

	//Write style for screen_bar_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_4, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_bar_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_bar_4, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_bar_4, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_bar_4, lv_color_hex(0xfff700), LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_bar_4, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_bar_4, 0, LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write codes screen_mph_label
	ui->screen_mph_label = lv_label_create(ui->screen);
	lv_label_set_text(ui->screen_mph_label, "15");
	lv_label_set_long_mode(ui->screen_mph_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->screen_mph_label, 326, 589);
	lv_obj_set_size(ui->screen_mph_label, 100, 100);

	//Write style for screen_mph_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->screen_mph_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_mph_label, &lv_font_montserratMedium_50, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->screen_mph_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->screen_mph_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->screen_mph_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->screen_mph_label, 252, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_mph_label, lv_color_hex(0x253142), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_mph_label, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_mph_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_temp_label
	ui->screen_temp_label = lv_label_create(ui->screen);
	lv_label_set_text(ui->screen_temp_label, "15°C");
	lv_label_set_long_mode(ui->screen_temp_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->screen_temp_label, -3, 613);
	lv_obj_set_size(ui->screen_temp_label, 105, 39);

	//Write style for screen_temp_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->screen_temp_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_temp_label, &lv_font_montserratMedium_22, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->screen_temp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->screen_temp_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->screen_temp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->screen_temp_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_temp_label, lv_color_hex(0x181E2B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_temp_label, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_temp_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_tripA_miles_label
	ui->screen_tripA_miles_label = lv_label_create(ui->screen);
	lv_label_set_text(ui->screen_tripA_miles_label, "128.8");
	lv_label_set_long_mode(ui->screen_tripA_miles_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->screen_tripA_miles_label, 595, 321);
	lv_obj_set_size(ui->screen_tripA_miles_label, 100, 57);

	//Write style for screen_tripA_miles_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->screen_tripA_miles_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_tripA_miles_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->screen_tripA_miles_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->screen_tripA_miles_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->screen_tripA_miles_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->screen_tripA_miles_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_tripA_miles_label, lv_color_hex(0x181E2B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_tripA_miles_label, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_tripA_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_tripB_miles_label
	ui->screen_tripB_miles_label = lv_label_create(ui->screen);
	lv_label_set_text(ui->screen_tripB_miles_label, "78.8");
	lv_label_set_long_mode(ui->screen_tripB_miles_label, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->screen_tripB_miles_label, 595, 929);
	lv_obj_set_size(ui->screen_tripB_miles_label, 100, 38);

	//Write style for screen_tripB_miles_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->screen_tripB_miles_label, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_tripB_miles_label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->screen_tripB_miles_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->screen_tripB_miles_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->screen_tripB_miles_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->screen_tripB_miles_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_tripB_miles_label, lv_color_hex(0x202A38), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_tripB_miles_label, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_tripB_miles_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_label_1
	ui->screen_label_1 = lv_label_create(ui->screen);
	lv_label_set_text(ui->screen_label_1, "3");
	lv_label_set_long_mode(ui->screen_label_1, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->screen_label_1, 673, 617);
	lv_obj_set_size(ui->screen_label_1, 38, 41);

	//Write style for screen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->screen_label_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->screen_label_1, &lv_font_montserratMedium_25, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->screen_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->screen_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->screen_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->screen_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_label_1, lv_color_hex(0x181E2B), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_label_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write codes screen_left_btm_arc
	ui->screen_left_btm_arc = lv_arc_create(ui->screen);
	lv_arc_set_mode(ui->screen_left_btm_arc, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui->screen_left_btm_arc, 0, 1000);
	lv_arc_set_bg_angles(ui->screen_left_btm_arc, 198, 246);
	lv_arc_set_value(ui->screen_left_btm_arc, 100);
	lv_arc_set_rotation(ui->screen_left_btm_arc, 20);
	lv_obj_set_style_arc_rounded(ui->screen_left_btm_arc, 0,  LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_rounded(ui->screen_left_btm_arc, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->screen_left_btm_arc, 89, 368);
	lv_obj_set_size(ui->screen_left_btm_arc, 566, 543);

	//Write style for screen_left_btm_arc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_left_btm_arc, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_left_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_left_btm_arc, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->screen_left_btm_arc, 25, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_left_btm_arc, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->screen_left_btm_arc, lv_color_hex(0xFF6379), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for screen_left_btm_arc, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_left_btm_arc, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->screen_left_btm_arc, 5, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes screen_left_top_arc
	ui->screen_left_top_arc = lv_arc_create(ui->screen);
	lv_arc_set_mode(ui->screen_left_top_arc, LV_ARC_MODE_NORMAL);
	lv_arc_set_range(ui->screen_left_top_arc, 0, 1000);
	lv_arc_set_bg_angles(ui->screen_left_top_arc, 198, 245);
	lv_arc_set_value(ui->screen_left_top_arc, 100);
	lv_arc_set_rotation(ui->screen_left_top_arc, 77);
	lv_obj_set_style_arc_rounded(ui->screen_left_top_arc, 0,  LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_rounded(ui->screen_left_top_arc, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->screen_left_top_arc, 89, 368);
	lv_obj_set_size(ui->screen_left_top_arc, 566, 543);

	//Write style for screen_left_top_arc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_left_top_arc, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_left_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_left_top_arc, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->screen_left_top_arc, 25, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_left_top_arc, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->screen_left_top_arc, lv_color_hex(0xFF8801), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for screen_left_top_arc, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_left_top_arc, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->screen_left_top_arc, 5, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes screen_right_btm_arc
	ui->screen_right_btm_arc = lv_arc_create(ui->screen);
	lv_arc_set_mode(ui->screen_right_btm_arc, LV_ARC_MODE_REVERSE);
	lv_arc_set_range(ui->screen_right_btm_arc, 0, 1000);
	lv_arc_set_bg_angles(ui->screen_right_btm_arc, 86, 134);
	lv_arc_set_value(ui->screen_right_btm_arc, 100);
	lv_arc_set_rotation(ui->screen_right_btm_arc, 8);
	lv_obj_set_style_arc_rounded(ui->screen_right_btm_arc, 0,  LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_rounded(ui->screen_right_btm_arc, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->screen_right_btm_arc, 89, 368);
	lv_obj_set_size(ui->screen_right_btm_arc, 566, 543);

	//Write style for screen_right_btm_arc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_right_btm_arc, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_right_btm_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_right_btm_arc, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->screen_right_btm_arc, 25, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_right_btm_arc, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->screen_right_btm_arc, lv_color_hex(0xff6500), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for screen_right_btm_arc, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_right_btm_arc, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->screen_right_btm_arc, 5, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes screen_right_top_arc
	ui->screen_right_top_arc = lv_arc_create(ui->screen);
	lv_arc_set_mode(ui->screen_right_top_arc, LV_ARC_MODE_REVERSE);
	lv_arc_set_range(ui->screen_right_top_arc, 0, 1000);
	lv_arc_set_bg_angles(ui->screen_right_top_arc, 38, 85);
	lv_arc_set_value(ui->screen_right_top_arc, 100);
	lv_arc_set_rotation(ui->screen_right_top_arc, 0);
	lv_obj_set_style_arc_rounded(ui->screen_right_top_arc, 0,  LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_rounded(ui->screen_right_top_arc, 0, LV_STATE_DEFAULT);
	lv_obj_set_pos(ui->screen_right_top_arc, 89, 368);
	lv_obj_set_size(ui->screen_right_top_arc, 566, 543);

	//Write style for screen_right_top_arc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_right_top_arc, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_right_top_arc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_right_top_arc, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
	lv_obj_set_style_arc_width(ui->screen_right_top_arc, 25, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_opa(ui->screen_right_top_arc, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
	lv_obj_set_style_arc_color(ui->screen_right_top_arc, lv_color_hex(0x4FF5FF), LV_PART_INDICATOR|LV_STATE_DEFAULT);

	//Write style for screen_right_top_arc, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_right_top_arc, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_all(ui->screen_right_top_arc, 5, LV_PART_KNOB|LV_STATE_DEFAULT);


	//Write codes screen_sw_1
	ui->screen_sw_1 = lv_switch_create(ui->screen);
	lv_obj_set_pos(ui->screen_sw_1, 61, 567);
	lv_obj_set_size(ui->screen_sw_1, 156, 30);

	//Write style for screen_sw_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_sw_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->screen_sw_1, 155, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->screen_sw_1, lv_color_hex(0x4FF5FF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(ui->screen_sw_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_sw_1, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_sw_1, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->screen_sw_1, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->screen_sw_1, lv_color_hex(0x4FF5FF), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->screen_sw_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->screen_sw_1, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for screen_sw_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_sw_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_sw_1, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_sw_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_sw_1, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_sw_1, 17, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes screen_sw_2
	ui->screen_sw_2 = lv_switch_create(ui->screen);
	lv_obj_set_pos(ui->screen_sw_2, 61, 618);
	lv_obj_set_size(ui->screen_sw_2, 156, 30);

	//Write style for screen_sw_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_sw_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_sw_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->screen_sw_2, 155, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->screen_sw_2, lv_color_hex(0x4FF5FF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(ui->screen_sw_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_sw_2, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_sw_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_sw_2, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->screen_sw_2, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->screen_sw_2, lv_color_hex(0x4FF5FF), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->screen_sw_2, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->screen_sw_2, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for screen_sw_2, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_sw_2, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_sw_2, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_sw_2, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_sw_2, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_sw_2, 17, LV_PART_KNOB|LV_STATE_DEFAULT);

	//Write codes screen_sw_3
	ui->screen_sw_3 = lv_switch_create(ui->screen);
	lv_obj_set_pos(ui->screen_sw_3, 61, 669);
	lv_obj_set_size(ui->screen_sw_3, 156, 30);

	//Write style for screen_sw_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_sw_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_sw_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(ui->screen_sw_3, 155, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(ui->screen_sw_3, lv_color_hex(0x4FF5FF), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(ui->screen_sw_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_sw_3, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->screen_sw_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

	//Write style for screen_sw_3, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
	lv_obj_set_style_bg_opa(ui->screen_sw_3, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(ui->screen_sw_3, lv_color_hex(0x4FF5FF), LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_bg_grad_dir(ui->screen_sw_3, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_CHECKED);
	lv_obj_set_style_border_width(ui->screen_sw_3, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

	//Write style for screen_sw_3, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
	lv_obj_set_style_bg_opa(ui->screen_sw_3, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui->screen_sw_3, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_grad_dir(ui->screen_sw_3, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui->screen_sw_3, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->screen_sw_3, 17, LV_PART_KNOB|LV_STATE_DEFAULT);

	//The custom code of screen.
	

	//Update current screen layout.
	lv_obj_update_layout(ui->screen);

}
