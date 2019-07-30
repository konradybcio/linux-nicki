// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Pavel Dubrova <paveldubrova@gmail.com>
 *
 * Based on JDI model LT070ME05000 Panel driver
 */

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/display_timing.h>
#include <video/videomode.h>

// static const char * const regulator_names[] = {
// 	"vdd",
// 	"vddio",
// 	"vdda"
// };

struct jdc_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	//struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];
	struct regulator *vddio_supply;
	struct regulator *vdd_supply;
	struct regulator *vdda_supply;


	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
	//struct gpio_desc *te_gpio;
	//struct backlight_device *backlight;

	bool prepared;
	bool enabled;

	const struct drm_display_mode *mode;
};

static const u8 cmd_on1[2] = { 0xB0, 0x00 };
static const u8 cmd_on2[2] = { 0xD6, 0x01 };
static const u8 cmd_on3[25] =
		{
				0xC7, 0x07, 0x08, 0x0B, 0x12, 0x22, 0x3D, 0x30,
				0x3D, 0x52, 0x54, 0x68, 0x78, 0x07, 0x08, 0x0B,
				0x12, 0x22, 0x3D, 0x30, 0x3D, 0x52, 0x54, 0x68,
				0x78
		};
static const u8 cmd_on4[25] =
		{
				0xC8, 0x07, 0x12, 0x1A, 0x25, 0x32, 0x45, 0x33,
				0x3F, 0x4F, 0x50, 0x5D, 0x78, 0x07, 0x12, 0x1A,
				0x25, 0x32, 0x45, 0x33, 0x3F, 0x4F, 0x50, 0x5D,
				0x78
		};
static const u8 cmd_on5[25] =
		{
				0xC9, 0x07, 0x21, 0x2B, 0x33, 0x3D, 0x4C, 0x35,
				0x40, 0x51, 0x53, 0x5C, 0x78, 0x07, 0x21, 0x2B,
				0x33, 0x3D, 0x4C, 0x35, 0x40, 0x51, 0x53, 0x5C,
				0x78
		};

static inline struct jdc_panel *to_jdc(struct drm_panel *panel)
{
	return container_of(panel, struct jdc_panel, base);
}

static int jdc_panel_enable(struct drm_panel *panel)
{
	struct jdc_panel *jdc_panel = to_jdc(panel);

	if (jdc_panel->enabled)
		return 0;

	//backlight_enable(jdc->backlight);

	jdc_panel->enabled = true;

	return 0;
}

static int jdc_panel_init(struct jdc_panel *jdc_panel)
{
	struct device *dev = &jdc_panel->dsi->dev;
	ssize_t wr_sz = 0;
	int rc = 0;

	jdc_panel->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	wr_sz = mipi_dsi_generic_write(jdc_panel->dsi, cmd_on1, sizeof(cmd_on1));
	if (wr_sz < 0){
		pr_info("1\n");
		return wr_sz;
	}

	wr_sz = mipi_dsi_generic_write(jdc_panel->dsi, cmd_on2, sizeof(cmd_on2));
	if (wr_sz < 0){
		pr_info("2\n");
		return wr_sz;
	}

	wr_sz = mipi_dsi_generic_write(jdc_panel->dsi, cmd_on3, sizeof(cmd_on3));
	if (wr_sz < 0)
		{pr_info("3\n");
		return wr_sz;
	}
	

	wr_sz = mipi_dsi_generic_write(jdc_panel->dsi, cmd_on4, sizeof(cmd_on4));
	if (wr_sz < 0)
		{pr_info("4\n");
		return wr_sz;
	}
	

	wr_sz = mipi_dsi_generic_write(jdc_panel->dsi, cmd_on5, sizeof(cmd_on5));
	if (wr_sz < 0)
		{pr_info("5\n");
		return wr_sz;
	}

	rc = mipi_dsi_dcs_exit_sleep_mode(jdc_panel->dsi);
	if (rc < 0) {
		dev_err(dev, "Cannot send exit sleep cmd: %d\n", rc);
		return rc;
	}

	msleep(120);

	return rc;
}

static int jdc_panel_on(struct jdc_panel *jdc_panel)
{
	struct device *dev = &jdc_panel->dsi->dev;
	int rc = 0;

	jdc_panel->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	rc = mipi_dsi_dcs_set_display_on(jdc_panel->dsi);
	if (rc < 0){
		dev_err(dev, "Failed to set display on: %d\n", rc);
		return rc;
	}

	msleep(120);

	return rc;
}

static int jdc_panel_disable(struct drm_panel *panel)
{
	struct jdc_panel *jdc_panel = to_jdc(panel);

	if (!jdc_panel->enabled)
		return 0;

	//backlight_disable(jdc->backlight);

	jdc_panel->enabled = false;

	return 0;
}

static void jdc_panel_off(struct jdc_panel *jdc_panel)
{
	struct device *dev = &jdc_panel->dsi->dev;
	int rc;

	jdc_panel->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	rc = mipi_dsi_dcs_set_display_off(jdc_panel->dsi);
	if (rc < 0)
		dev_err(dev, "Failed to set display off: %d\n", rc);

	rc = mipi_dsi_dcs_enter_sleep_mode(jdc_panel->dsi);
	if (rc < 0)
		dev_err(dev, "Failed to enter sleep mode: %d\n", rc);

	msleep(100);
	return rc;
}


static int jdc_panel_unprepare(struct drm_panel *panel)
{
	struct jdc_panel *jdc_panel = to_jdc(panel);
	int rc = 0;

	if (!jdc_panel->prepared)
		return 0;

	jdc_panel_off(jdc_panel);

	// rc = regulator_bulk_disable(ARRAY_SIZE(jdc->supplies), jdc->supplies);
	// if (rc < 0)
	// 	dev_err(dev, "regulator disable failed, %d\n", rc);

	// gpiod_set_value(jdc->enable_gpio, 0);

	// gpiod_set_value(jdc->reset_gpio, 1);

	//gpiod_set_value(jdc->te_gpio, 0);

	jdc_panel->prepared = false;

	return rc;
}

static int jdc_panel_prepare(struct drm_panel *panel)
{
	struct jdc_panel *jdc_panel = to_jdc(panel);
	struct device *dev = &jdc_panel->dsi->dev;
	int rc;

	if (jdc_panel->prepared)
		return 0;

	// ret = regulator_bulk_enable(ARRAY_SIZE(jdc->supplies), jdc->supplies);
	// if (ret < 0) {
	// 	dev_err(dev, "regulator enable failed, %d\n", ret);
	// 	return ret;
	// }

	/* VDDIO */
	rc = regulator_enable(jdc_panel->vddio_supply);
	if (rc < 0){
		pr_info("PANEL VDDIO ENABLE\n");
		return rc;
	}

 	msleep(80);
	
	/* VDDA */
	rc = regulator_enable(jdc_panel->vdda_supply);
	if (rc < 0){
		pr_info("PANEL VDDA ENABLE\n");
		return rc;
	}
	
	msleep(80);

	/* VDD */
	rc = regulator_enable(jdc_panel->vdd_supply);
	if (rc < 0){
		pr_info("PANEL VDD ENABLE\n");
		return rc;
	}

	msleep(120);

	//gpiod_set_value(jdc->te_gpio, 1);
	//usleep_range(10, 20);

	gpiod_set_value(jdc_panel->reset_gpio, 1);
	usleep_range(10, 20);

	gpiod_set_value(jdc_panel->enable_gpio, 1);
	usleep_range(10, 20);

	rc = jdc_panel_init(jdc_panel);
	if (rc < 0) {
		dev_err(dev, "failed to init panel: %d\n", rc);
		goto poweroff;
	}

	rc = jdc_panel_on(jdc_panel);
	if (rc < 0) {
		dev_err(dev, "failed to set panel on: %d\n", rc);
		goto poweroff;
	}

	jdc_panel->prepared = true;

	return 0;

poweroff:
	// ret = regulator_bulk_disable(ARRAY_SIZE(jdc->supplies), jdc->supplies);
	// if (ret < 0)
	// 	dev_err(dev, "regulator disable failed, %d\n", ret);

	regulator_disable(jdc_panel->vdd_supply);
	regulator_disable(jdc_panel->vdda_supply);
	regulator_disable(jdc_panel->vddio_supply);

	// gpiod_set_value(jdc->enable_gpio, 0);

	// gpiod_set_value(jdc->reset_gpio, 1);

	//gpiod_set_value(jdc->te_gpio, 0);

	return rc;
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
	struct jdc_panel *jdc_panel = to_jdc(panel);
	struct device *dev = &jdc_panel->dsi->dev;

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

static int jdc_panel_add(struct jdc_panel *jdc_panel)
{
	struct device *dev = &jdc_panel->dsi->dev;
	int rc;

	jdc_panel->mode = &default_mode;

	// for (i = 0; i < ARRAY_SIZE(jdc->supplies); i++)
	// 	jdc->supplies[i].supply = regulator_names[i];

	// ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(jdc->supplies),
	// 			      jdc->supplies);
	// if (ret < 0) {
	// 	dev_err(dev, "Failed to init regulator, ret=%d\n", ret);
	// 	return ret;
	// }

	jdc_panel->vddio_supply = devm_regulator_get(dev, "vddio");
	if (IS_ERR(jdc_panel->vddio_supply)) {
		dev_err(dev, "cannot get vddio regulator: %ld\n",
			PTR_ERR(jdc_panel->vddio_supply));
		return PTR_ERR(jdc_panel->vddio_supply);
	}

	jdc_panel->vdda_supply = devm_regulator_get_optional(dev, "vdda");
	if (IS_ERR(jdc_panel->vdda_supply)) {
		dev_err(dev, "cannot get vdda regulator: %ld\n",
			PTR_ERR(jdc_panel->vdda_supply));
		jdc_panel->vdda_supply = NULL;
	}
	
	jdc_panel->vdd_supply = devm_regulator_get_optional(dev, "vdd");
	if (IS_ERR(jdc_panel->vdd_supply)) {
		dev_err(dev, "cannot get vdd regulator: %ld\n",
			PTR_ERR(jdc_panel->vdd_supply));
		jdc_panel->vdd_supply = NULL;
	}

	jdc_panel->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_ASIS);
	if (IS_ERR(jdc_panel->enable_gpio)) {
		rc = PTR_ERR(jdc_panel->enable_gpio);
		dev_err(dev, "Cannot get enable-gpio %d\n", rc);
		return rc;
	}

	jdc_panel->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_ASIS);
	if (IS_ERR(jdc_panel->reset_gpio)) {
		rc = PTR_ERR(jdc_panel->reset_gpio);
		dev_err(dev, "Cannot get reset-gpio %d\n", rc);
		return rc;
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

	drm_panel_init(&jdc_panel->base);
	jdc_panel->base.funcs = &jdc_panel_funcs;
	jdc_panel->base.dev = dev;

	rc = drm_panel_add(&jdc_panel->base);
	if (rc < 0)
		pr_err("drm panel add failed\n");

	return rc;
}

static void jdc_panel_del(struct jdc_panel *jdc_panel)
{
	if (jdc_panel->base.dev)
		drm_panel_remove(&jdc_panel->base);
}

static int jdc_panel_probe(struct mipi_dsi_device *dsi)
{
	struct jdc_panel *jdc_panel;
	int rc;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_MODE_EOT_PACKET; //MIPI_DSI_CLOCK_NON_CONTINUOUS

	// jdc = devm_kzalloc(&dsi->dev, sizeof(*jdc), GFP_KERNEL);
	// if (!jdc)
	// 	return -ENOMEM;
	jdc_panel = devm_kzalloc(&dsi->dev,
				sizeof(*jdc_panel), GFP_KERNEL);
	if (!jdc_panel)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, jdc_panel);
	jdc_panel->dsi = dsi;

	rc = jdc_panel_add(jdc_panel);
	if (rc < 0)
		return rc;

	return mipi_dsi_attach(dsi);
}

static int jdc_panel_remove(struct mipi_dsi_device *dsi)
{
	struct jdc_panel *jdc_panel = mipi_dsi_get_drvdata(dsi);
	struct device *dev = &jdc_panel->dsi->dev;
	int ret;

	ret = jdc_panel_disable(&jdc_panel->base);
	if (ret < 0)
		dev_err(dev, "Failed to disable panel: %d\n", ret);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(dev, "Failed to detach from DSI host: %d\n",
			ret);

	jdc_panel_del(jdc_panel);

	return 0;
}

static void jdc_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct jdc_panel *jdc_panel = mipi_dsi_get_drvdata(dsi);

	jdc_panel_disable(&jdc_panel->base);
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
