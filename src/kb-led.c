// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * kb_led.c
 *
 * Copyright (C) 2017 Jeremy Soller <jeremy@system76.com>
 */

#define SET_KB_LED 0x67

union kb_led_color {
	u32 rgb;
	struct { u32 b:8, g:8, r:8, : 8; };
};

enum kb_led_region {
	KB_LED_REGION_LEFT,
	KB_LED_REGION_CENTER,
	KB_LED_REGION_RIGHT,
	KB_LED_REGION_EXTRA,
};

static enum led_brightness kb_led_brightness;

static enum led_brightness kb_led_toggle_brightness = 72;

static enum led_brightness kb_led_levels[] = { 48, 72, 96, 144, 192, 255 };

static union kb_led_color kb_led_regions[] = {
	{ .rgb = 0xFFFFFF },
	{ .rgb = 0xFFFFFF },
	{ .rgb = 0xFFFFFF },
	{ .rgb = 0xFFFFFF }
};

static int kb_led_colors_i;

static union kb_led_color kb_led_colors[] = {
	{ .rgb = 0xFFFFFF },
	{ .rgb = 0x0000FF },
	{ .rgb = 0xFF0000 },
	{ .rgb = 0xFF00FF },
	{ .rgb = 0x00FF00 },
	{ .rgb = 0x00FFFF },
	{ .rgb = 0xFFFF00 }
};

static enum led_brightness kb_led_get(struct led_classdev *led_cdev)
{
	return kb_led_brightness;
}

static int kb_led_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	pr_debug("%s %d\n", __func__, (int)value);

	if (!s76_wmbb(SET_KB_LED, 0xF4000000 | value, NULL)) {
		kb_led_brightness = value;
	}

	return 0;
}

static void kb_led_color_set_wmi(enum kb_led_region region, union kb_led_color color)
{
	u32 cmd;

	pr_debug("%s %d %06X\n", __func__, (int)region, (int)color.rgb);

	switch (region) {
	case KB_LED_REGION_LEFT:
		cmd = 0xF0000000;
		break;
	case KB_LED_REGION_CENTER:
		cmd = 0xF1000000;
		break;
	case KB_LED_REGION_RIGHT:
		cmd = 0xF2000000;
		break;
	case KB_LED_REGION_EXTRA:
		cmd = 0xF3000000;
		break;
	default:
		return;
	}

	cmd |= color.b << 16;
	cmd |= color.r <<  8;
	cmd |= color.g <<  0;

	if (!s76_wmbb(SET_KB_LED, cmd, NULL)) {
		kb_led_regions[region] = color;
	}
}

// HACK: Directly call ECMD to fix serw14
static void kb_led_color_set(enum kb_led_region region, union kb_led_color color)
{
	struct acpi_object_list input;
	union acpi_object obj;
	acpi_handle handle;
	acpi_status status;
	u8 *buf;

	buf = kzalloc(8, GFP_KERNEL);

	pr_debug("%s %d %06X\n", __func__, (int)region, (int)color.rgb);

	buf[0] = 5;
	buf[2] = 0xCA;
	buf[4] = color.b;
	buf[5] = color.r;
	buf[6] = color.g;

	switch (region) {
	case KB_LED_REGION_LEFT:
		buf[3] = 0x03;
		break;
	case KB_LED_REGION_CENTER:
		buf[3] = 0x04;
		break;
	case KB_LED_REGION_RIGHT:
		buf[3] = 0x05;
		break;
	case KB_LED_REGION_EXTRA:
		buf[3] = 0x0B;
		break;
	}

	obj.type = ACPI_TYPE_BUFFER;
	obj.buffer.length = 8;
	obj.buffer.pointer = buf;

	input.count = 1;
	input.pointer = &obj;

	status = acpi_get_handle(NULL, (acpi_string)"\\_SB.PC00.LPCB.EC", &handle);
	if (ACPI_FAILURE(status)) {
		pr_err("%s failed to get handle: %x\n", __func__, status);
		return;
	}

	status = acpi_evaluate_object(handle, "ECMD", &input, NULL);
	if (ACPI_FAILURE(status)) {
		pr_err("%s failed to call EC_CMD: %x\n", __func__, status);
		return;
	}

	// Update lightbar to match keyboard color
	buf[3] = 0x07;
	status = acpi_evaluate_object(handle, "ECMD", &input, NULL);
	if (ACPI_FAILURE(status)) {
		pr_err("%s failed to call EC_CMD: %x\n", __func__, status);
		return;
	}

	kfree(buf);
	kb_led_regions[region] = color;
}

static struct led_classdev kb_led = {
	.name = "system76::kbd_backlight",
	.flags = LED_BRIGHT_HW_CHANGED,
	.brightness_get = kb_led_get,
	.brightness_set_blocking = kb_led_set,
	.max_brightness = 255,
};

static ssize_t kb_led_color_show(enum kb_led_region region, char *buf)
{
	return sysfs_emit(buf, "%06X\n", (int)kb_led_regions[region].rgb);
}

static ssize_t kb_led_color_store(enum kb_led_region region, const char *buf, size_t size)
{
	unsigned int val;
	int ret;
	union kb_led_color color;

	ret = kstrtouint(buf, 16, &val);
	if (ret) {
		return ret;
	}

	color.rgb = (u32)val;
	if (driver_flags & DRIVER_KB_LED_WMI)
		kb_led_color_set_wmi(region, color);
	else
		kb_led_color_set(region, color);

	return size;
}

static ssize_t kb_led_color_left_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return kb_led_color_show(KB_LED_REGION_LEFT, buf);
}

static ssize_t kb_led_color_left_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return kb_led_color_store(KB_LED_REGION_LEFT, buf, size);
}

static struct device_attribute kb_led_color_left_dev_attr = {
	.attr = {
		.name = "color_left",
		.mode = 0644,
	},
	.show = kb_led_color_left_show,
	.store = kb_led_color_left_store,
};

static ssize_t kb_led_color_center_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return kb_led_color_show(KB_LED_REGION_CENTER, buf);
}

static ssize_t kb_led_color_center_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return kb_led_color_store(KB_LED_REGION_CENTER, buf, size);
}

static struct device_attribute kb_led_color_center_dev_attr = {
	.attr = {
		.name = "color_center",
		.mode = 0644,
	},
	.show = kb_led_color_center_show,
	.store = kb_led_color_center_store,
};

static ssize_t kb_led_color_right_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return kb_led_color_show(KB_LED_REGION_RIGHT, buf);
}

static ssize_t kb_led_color_right_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return kb_led_color_store(KB_LED_REGION_RIGHT, buf, size);
}

static struct device_attribute kb_led_color_right_dev_attr = {
	.attr = {
		.name = "color_right",
		.mode = 0644,
	},
	.show = kb_led_color_right_show,
	.store = kb_led_color_right_store,
};

static ssize_t kb_led_color_extra_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return kb_led_color_show(KB_LED_REGION_EXTRA, buf);
}

static ssize_t kb_led_color_extra_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return kb_led_color_store(KB_LED_REGION_EXTRA, buf, size);
}

static struct device_attribute kb_led_color_extra_dev_attr = {
	.attr = {
		.name = "color_extra",
		.mode = 0644,
	},
	.show = kb_led_color_extra_show,
	.store = kb_led_color_extra_store,
};

static void kb_led_enable(void)
{
	pr_debug("%s\n", __func__);

	s76_wmbb(SET_KB_LED, 0xE007F001, NULL);
}

static void kb_led_disable(void)
{
	pr_debug("%s\n", __func__);

	s76_wmbb(SET_KB_LED, 0xE0003001, NULL);
}

static void kb_led_suspend(void)
{
	pr_debug("%s\n", __func__);

	// Disable keyboard backlight
	kb_led_disable();
}

static void kb_led_resume(void)
{
	enum kb_led_region region;

	pr_debug("%s\n", __func__);

	// Disable keyboard backlight
	kb_led_disable();

	// Reset current color
	for (region = 0; region < sizeof(kb_led_regions)/sizeof(union kb_led_color); region++) {
		if (driver_flags & DRIVER_KB_LED_WMI)
			kb_led_color_set_wmi(region, kb_led_regions[region]);
		else
			kb_led_color_set(region, kb_led_regions[region]);
	}

	// Reset current brightness
	kb_led_set(&kb_led, kb_led_brightness);

	// Enable keyboard backlight
	kb_led_enable();
}

static int __init kb_led_init(struct device *dev)
{
	int err;

	err = devm_led_classdev_register(dev, &kb_led);
	if (unlikely(err)) {
		return err;
	}

	if (device_create_file(kb_led.dev, &kb_led_color_left_dev_attr) != 0) {
		pr_err("failed to create kb_led_color_left\n");
	}

	if (device_create_file(kb_led.dev, &kb_led_color_center_dev_attr) != 0) {
		pr_err("failed to create kb_led_color_center\n");
	}

	if (device_create_file(kb_led.dev, &kb_led_color_right_dev_attr) != 0) {
		pr_err("failed to create kb_led_color_right\n");
	}

	if (device_create_file(kb_led.dev, &kb_led_color_extra_dev_attr) != 0) {
		pr_err("failed to create kb_led_color_extra\n");
	}

	kb_led_resume();

	return 0;
}

static void __exit kb_led_exit(void)
{
	device_remove_file(kb_led.dev, &kb_led_color_extra_dev_attr);
	device_remove_file(kb_led.dev, &kb_led_color_right_dev_attr);
	device_remove_file(kb_led.dev, &kb_led_color_center_dev_attr);
	device_remove_file(kb_led.dev, &kb_led_color_left_dev_attr);
}

static void kb_wmi_brightness(enum led_brightness value)
{
	pr_debug("%s %d\n", __func__, (int)value);

	kb_led_set(&kb_led, value);
	led_classdev_notify_brightness_hw_changed(&kb_led, value);
}

static void kb_wmi_toggle(void)
{
	if (kb_led_brightness > 0) {
		kb_led_toggle_brightness = kb_led_brightness;
		kb_wmi_brightness(LED_OFF);
	} else {
		kb_wmi_brightness(kb_led_toggle_brightness);
	}
}

static void kb_wmi_dec(void)
{
	int i;

	if (kb_led_brightness > 0) {
		for (i = sizeof(kb_led_levels)/sizeof(enum led_brightness); i > 0; i--) {
			if (kb_led_levels[i - 1] < kb_led_brightness) {
				kb_wmi_brightness(kb_led_levels[i - 1]);
				break;
			}
		}
	} else {
		kb_wmi_toggle();
	}
}

static void kb_wmi_inc(void)
{
	int i;

	if (kb_led_brightness > 0) {
		for (i = 0; i < sizeof(kb_led_levels)/sizeof(enum led_brightness); i++) {
			if (kb_led_levels[i] > kb_led_brightness) {
				kb_wmi_brightness(kb_led_levels[i]);
				break;
			}
		}
	} else {
		kb_wmi_toggle();
	}
}

static void kb_wmi_color(void)
{
	enum kb_led_region region;

	kb_led_colors_i += 1;
	if (kb_led_colors_i >= sizeof(kb_led_colors)/sizeof(union kb_led_color)) {
		kb_led_colors_i = 0;
	}

	for (region = 0; region < sizeof(kb_led_regions)/sizeof(union kb_led_color); region++) {
		if (driver_flags & DRIVER_KB_LED_WMI)
			kb_led_color_set_wmi(region, kb_led_colors[kb_led_colors_i]);
		else
			kb_led_color_set(region, kb_led_colors[kb_led_colors_i]);
	}

	led_classdev_notify_brightness_hw_changed(&kb_led, kb_led_brightness);
}
