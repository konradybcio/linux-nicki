// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Pavel Dubrova <paveldubrova@gmail.com>
 *
 * Based on JDI model LT070ME05000 Panel driver
 */

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

static const char * const regulator_names[] = {
	"vdd",
	"vddio",
	"vdda"
};

struct jdc_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];

	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
	//struct gpio_desc *te_gpio;
	//struct backlight_device *backlight;

	bool prepared;
	bool enabled;

	const struct drm_display_mode *mode;
};

//static const u8 cmd_on1[2] = { 0xB0, 0x04 };
static const u8 cmd_on2[4] = { 0x52, 0x54, 0x68, 0x78 };
static const u8 cmd_on3[4] = { 0x4F, 0x50, 0x5D, 0x78 };
static const u8 cmd_on4[4] = { 0x51, 0x53, 0x5C, 0x78 };

static inline struct jdc_panel *to_jdc_panel(struct drm_panel *panel)
{
	return container_of(panel, struct jdc_panel, base);
}

static int jdc_panel_init(struct jdc_panel *jdc)
{
	struct mipi_dsi_device *dsi = jdc->dsi;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_write(dsi, 0xB0, (u8[]){ 0x00 }, 1);
	if (ret < 0){
		pr_info("1\n");
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD6, (u8[]){ 0x01 }, 1);
	if (ret < 0){
		pr_info("2\n");
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xC7, NULL, 0);
	if (ret < 0)
		{pr_info("3\n");
		return ret;
	}
	

	ret = mipi_dsi_dcs_write(dsi, 0x3D, cmd_on2, sizeof(cmd_on2));
	if (ret < 0)
		{pr_info("4\n");
		return ret;
	}
	

	ret = mipi_dsi_dcs_write(dsi, 0xC8, NULL, 0);
	if (ret < 0)
		{pr_info("5\n");
		return ret;
	}
	

	ret = mipi_dsi_dcs_write(dsi, 0x3F, cmd_on3, sizeof(cmd_on3));
	if (ret < 0)
		{pr_info("6\n");
		return ret;
	}
	

	ret = mipi_dsi_dcs_write(dsi, 0xC9, NULL, 0);
	if (ret < 0)
		{pr_info("7\n");
		return ret;
	}
	

	ret = mipi_dsi_dcs_write(dsi, 0x40, cmd_on4, sizeof(cmd_on4));
	if (ret < 0)
		{pr_info("8\n");
		return ret;
	}
	

	msleep(1);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0)
		return ret;

	msleep(30);

	return ret;
}

static int jdc_panel_on(struct jdc_panel *jdc)
{
	struct mipi_dsi_device *dsi = jdc->dsi;
	struct device *dev = &jdc->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0)
		dev_err(dev, "Failed to set display on: %d\n", ret);

	return ret;
}

static void jdc_panel_off(struct jdc_panel *jdc)
{
	struct mipi_dsi_device *dsi = jdc->dsi;
	struct device *dev = &jdc->dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
		dev_err(dev, "Failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);

	msleep(100);
}

static int jdc_panel_disable(struct drm_panel *panel)
{
	struct jdc_panel *jdc = to_jdc_panel(panel);

	if (!jdc->enabled)
		return 0;

	//backlight_disable(jdc->backlight);

	jdc->enabled = false;

	return 0;
}

static int jdc_panel_unprepare(struct drm_panel *panel)
{
	struct jdc_panel *jdc = to_jdc_panel(panel);
	struct device *dev = &jdc->dsi->dev;
	int ret;

	if (!jdc->prepared)
		return 0;

	jdc_panel_off(jdc);

	ret = regulator_bulk_disable(ARRAY_SIZE(jdc->supplies), jdc->supplies);
	if (ret < 0)
		dev_err(dev, "regulator disable failed, %d\n", ret);

	gpiod_set_value(jdc->enable_gpio, 0);

	gpiod_set_value(jdc->reset_gpio, 1);

	//gpiod_set_value(jdc->te_gpio, 0);

	jdc->prepared = false;

	return 0;
}

static int jdc_panel_prepare(struct drm_panel *panel)
{
	struct jdc_panel *jdc = to_jdc_panel(panel);
	struct device *dev = &jdc->dsi->dev;
	int ret;

	if (jdc->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(jdc->supplies), jdc->supplies);
	if (ret < 0) {
		dev_err(dev, "regulator enable failed, %d\n", ret);
		return ret;
	}

	msleep(20);

	//gpiod_set_value(jdc->te_gpio, 1);
	//usleep_range(10, 20);

	gpiod_set_value(jdc->reset_gpio, 0);
	usleep_range(10, 20);

	gpiod_set_value(jdc->enable_gpio, 1);
	usleep_range(10, 20);

	ret = jdc_panel_init(jdc);
	if (ret < 0) {
		dev_err(dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	ret = jdc_panel_on(jdc);
	if (ret < 0) {
		dev_err(dev, "failed to set panel on: %d\n", ret);
		goto poweroff;
	}

	jdc->prepared = true;

	return 0;

poweroff:
	ret = regulator_bulk_disable(ARRAY_SIZE(jdc->supplies), jdc->supplies);
	if (ret < 0)
		dev_err(dev, "regulator disable failed, %d\n", ret);

	gpiod_set_value(jdc->enable_gpio, 0);

	gpiod_set_value(jdc->reset_gpio, 1);

	//gpiod_set_value(jdc->te_gpio, 0);

	return ret;
}

static int jdc_panel_enable(struct drm_panel *panel)
{
	struct jdc_panel *jdc = to_jdc_panel(panel);

	if (jdc->enabled)
		return 0;

	//backlight_enable(jdc->backlight);

	jdc->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 149614,
	.hdisplay = 1080,
	.hsync_start = 1080 + 128,
	.hsync_end = 1080 + 128 + 8,
	.htotal = 1080 + 128 + 8 + 72,
	.vdisplay = 1920,
	.vsync_start = 1920 + 8,
	.vsync_end = 1920 + 8 + 4,
	.vtotal = 1920 + 8 + 4 + 4,
	.vrefresh = 60,
	// .flags = 0,
};

static int jdc_panel_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;
	struct jdc_panel *jdc = to_jdc_panel(panel);
	struct device *dev = &jdc->dsi->dev;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = 62;
	panel->connector->display_info.height_mm = 110;

	return 1;
}
/*
static int dsi_dcs_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, bl->props.brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static int dsi_dcs_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int ret;
	u16 brightness = bl->props.brightness;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}
*/
/*
static const struct backlight_ops dsi_bl_ops = {
	.update_status = dsi_dcs_bl_update_status,
	.get_brightness = dsi_dcs_bl_get_brightness,
};


static struct backlight_device *
drm_panel_create_dsi_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_properties props;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.brightness = 255;
	props.max_brightness = 255;

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &dsi_bl_ops, &props);
}
*/
static const struct drm_panel_funcs jdc_panel_funcs = {
	.disable = jdc_panel_disable,
	.unprepare = jdc_panel_unprepare,
	.prepare = jdc_panel_prepare,
	.enable = jdc_panel_enable,
	.get_modes = jdc_panel_get_modes,
};

static const struct of_device_id jdc_of_match[] = {
	{ .compatible = "jdc,renesas-1080p-vid", },
	{ }
};
MODULE_DEVICE_TABLE(of, jdc_of_match);

static int jdc_panel_add(struct jdc_panel *jdc)
{
	struct device *dev = &jdc->dsi->dev;
	int ret;
	unsigned int i;

	jdc->mode = &default_mode;

	for (i = 0; i < ARRAY_SIZE(jdc->supplies); i++)
		jdc->supplies[i].supply = regulator_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(jdc->supplies),
				      jdc->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to init regulator, ret=%d\n", ret);
		return ret;
	}

	jdc->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(jdc->enable_gpio)) {
		ret = PTR_ERR(jdc->enable_gpio);
		dev_err(dev, "Cannot get enable-gpio %d\n", ret);
		return ret;
	}

	jdc->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(jdc->reset_gpio)) {
		ret = PTR_ERR(jdc->reset_gpio);
		dev_err(dev, "Cannot get reset-gpio %d\n", ret);
		return ret;
	}
/*
	jdc->te_gpio = devm_gpiod_get(dev, "te", GPIOD_OUT_LOW);
	if (IS_ERR(jdc->te_gpio)) {
		ret = PTR_ERR(jdc->te_gpio);
		dev_err(dev, "Cannot get te-gpio %d\n", ret);
		return ret;
	}
*/

/*
	jdc->backlight = drm_panel_create_dsi_backlight(jdc->dsi);
	if (IS_ERR(jdc->backlight)) {
		ret = PTR_ERR(jdc->backlight);
		dev_err(dev, "Failed to register backlight %d\n", ret);
		return ret;
	}
	*/

	drm_panel_init(&jdc->base);
	jdc->base.funcs = &jdc_panel_funcs;
	jdc->base.dev = &jdc->dsi->dev;

	ret = drm_panel_add(&jdc->base);

	return ret;
}

static void jdc_panel_del(struct jdc_panel *jdc)
{
	if (jdc->base.dev)
		drm_panel_remove(&jdc->base);
}

static int jdc_panel_probe(struct mipi_dsi_device *dsi)
{
	struct jdc_panel *jdc;
	int ret;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_CLOCK_NON_CONTINUOUS; //| MIPI_DSI_MODE_EOT_PACKET; ???

	jdc = devm_kzalloc(&dsi->dev, sizeof(*jdc), GFP_KERNEL);
	if (!jdc)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, jdc);

	jdc->dsi = dsi;

	ret = jdc_panel_add(jdc);
	if (ret < 0)
		return ret;

	return mipi_dsi_attach(dsi);
}

static int jdc_panel_remove(struct mipi_dsi_device *dsi)
{
	struct jdc_panel *jdc = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = jdc_panel_disable(&jdc->base);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to disable panel: %d\n", ret);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n",
			ret);

	jdc_panel_del(jdc);

	return 0;
}

static void jdc_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct jdc_panel *jdc = mipi_dsi_get_drvdata(dsi);

	jdc_panel_disable(&jdc->base);
}

static struct mipi_dsi_driver jdc_panel_driver = {
	.driver = {
		.name = "panel-jdc-renesas-1080p",
		.of_match_table = jdc_of_match,
	},
	.probe = jdc_panel_probe,
	.remove = jdc_panel_remove,
	.shutdown = jdc_panel_shutdown,
};
module_mipi_dsi_driver(jdc_panel_driver);

MODULE_AUTHOR("Pavel Dubrova <pashadubrova@gmail.com>");
MODULE_DESCRIPTION("JDC Renesas 1080p DSI Panel Driver");
MODULE_LICENSE("GPL v2");
