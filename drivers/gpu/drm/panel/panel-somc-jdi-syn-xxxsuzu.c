/*
 * Copyright (C) 2019 AngeloGioacchino Del Regno <kholk11@gmail.com>
 *
 * from DT configurations on Sony Xperia Loire platform
 * XXXsuzu.0 IPS LCD Panel Driver
 * 
 * TODO: retrieve the correct panel name. It is a
 * hybris-incell panel featuring a JDI display 
 * with a Synaptics touchscreen.
 *
 * Copyright (c) 2016 Sony Mobile Communications Inc.
 * Parameters from dsi-panel-somc-synaptics-jdi-1080p-cmd.dtsi
 * 
 * Based on H455TAX01.0 IPS LCD Panel Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

//#define MDSS_BUG_SOLVED

struct XXXsuzu_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	struct backlight_device *backlight;

	struct regulator *vddio_supply;
	struct regulator *avdd_supply;
	struct regulator *tvdd_supply;

	struct gpio_desc *pan_reset_gpio;
	struct gpio_desc *ts_vddio_gpio;
	struct gpio_desc *ts_reset_gpio;

	bool prepared;
	bool enabled;

	const struct drm_display_mode *mode;
};

static const u8 cmd_unk1[2] = {0xb0, 0x00};
static const u8 cmd_unk2[2] = {0xd6, 0x01};
static const u8 cmd_on_unk3[3] = {0xc4, 0x70, 0x03};
static const u8 cmd_on_unk4[27] =
	{
		0xED, 0x27, 0x31, 0x2F, 0x13, 0x00, 0x6A, 0x99,
		0x03, 0x17, 0x91, 0xF2, 0x00, 0x00, 0x03, 0x14,
		0x17, 0x3F, 0x14, 0x12, 0x26, 0x23, 0x00, 0x20,
		0x00, 0x00, 0x57
	};
static const u8 cmd_unk5[27] = 
	{
		0xEE, 0x13, 0x61, 0x5F, 0x09, 0x00, 0x6A, 0x99,
		0x03, 0x00, 0x01, 0xB2, 0x00, 0x00, 0x03, 0x00,
		0x00, 0x33, 0x14, 0x12, 0x00, 0x21, 0x00, 0x20,
		0x00, 0x00, 0x57
	};
static const u8 cmd_on_unk6[27] = 
	{
		0xEF, 0x27, 0x31, 0x2F, 0x13, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x14,
		0x17, 0x0F, 0x14, 0x00, 0x00, 0x20, 0x00, 0x00,
		0x00, 0x00, 0xA6
	};
static const u8 cmd_on_unk7[27] = 
	{
		0xF0, 0xE3, 0x07, 0x73, 0xDF, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xE3, 0x00,
		0x00, 0x03, 0x14, 0x00, 0x00, 0x20, 0x00, 0x00,
		0x00, 0x00, 0xA7
	};
static const u8 cmd_on_unk8[2] = {0x35, 0x00};
static const u8 cmd_on_unk9[2] = {0x36, 0x00};
static const u8 cmd_on_unk10[2] = {0x3A, 0x77};
static const u8 cmd_on_unk11[5] = {0x2A, 0x00, 0x00, 0x04, 0x37};
static const u8 cmd_on_unk12[5] = {0x2B, 0x00, 0x00, 0x07, 0x7F};
static const u8 cmd_on_unk13[3] = {0x44, 0x00, 0x00};

static const u8 cmd_off_unk4[14] =
	{
		0xEC, 0x64, 0xDC, 0x7A, 0x7A, 0x3D, 0x00, 0x0B,
		0x0B, 0x13, 0x15, 0x68, 0x0B, 0x95,
	};

static inline struct XXXsuzu_panel *to_XXXsuzu(struct drm_panel *panel)
{
	return container_of(panel, struct XXXsuzu_panel, base);
}

static int XXXsuzu_panel_enable(struct drm_panel *panel)
{
	struct XXXsuzu_panel *XXXsuzu_panel = to_XXXsuzu(panel);

	if (XXXsuzu_panel->enabled)
		return 0;

	XXXsuzu_panel->enabled = true;

	return 0;
}

static int XXXsuzu_panel_init(struct XXXsuzu_panel *XXXsuzu_panel)
{
	struct device *dev = &XXXsuzu_panel->dsi->dev;
	ssize_t wr_sz = 0;
	int rc = 0;

	XXXsuzu_panel->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_unk1, sizeof(cmd_unk1));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 1: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_unk2, sizeof(cmd_unk2));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 2: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk3, sizeof(cmd_on_unk3));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 3: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk4, sizeof(cmd_on_unk4));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 4: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_unk5, sizeof(cmd_unk5));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 5: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk6, sizeof(cmd_on_unk6));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 6: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk7, sizeof(cmd_on_unk7));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 7: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk8, sizeof(cmd_on_unk8));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 8: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk9, sizeof(cmd_on_unk9));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 9: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk10, sizeof(cmd_on_unk10));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 10: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk11, sizeof(cmd_on_unk11));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 11: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk11, sizeof(cmd_on_unk12));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 12: %ld\n", wr_sz);

	wr_sz = mipi_dsi_generic_write(XXXsuzu_panel->dsi,
					cmd_on_unk11, sizeof(cmd_on_unk13));
	if (wr_sz < 0)
		dev_err(dev, "Cannot send ON command 13: %ld\n", wr_sz);

	rc = mipi_dsi_dcs_exit_sleep_mode(XXXsuzu_panel->dsi);
	if (rc < 0) {
		dev_err(dev, "Cannot send exit sleep cmd: %d\n", rc);
		return rc;
	}

	msleep(120);

	return rc;
}

static int XXXsuzu_panel_on(struct XXXsuzu_panel *XXXsuzu_panel)
{
	struct device *dev = &XXXsuzu_panel->dsi->dev;
	int rc = 0;

	rc = mipi_dsi_dcs_set_display_on(XXXsuzu_panel->dsi);
	if (rc < 0) {
		dev_err(dev, "Cannot send disp on cmd: %d\n", rc);
		return rc;
	}

	msleep(120);

	return rc;
}

static int XXXsuzu_panel_disable(struct drm_panel *panel)
{
	struct XXXsuzu_panel *XXXsuzu_panel = to_XXXsuzu(panel);

	if (!XXXsuzu_panel->enabled)
		return 0;

	XXXsuzu_panel->enabled = false;

	return 0;
}

static int XXXsuzu_panel_off(struct XXXsuzu_panel *XXXsuzu_panel)
{
	struct device *dev = &XXXsuzu_panel->dsi->dev;
	ssize_t wr_sz = 0;
	int rc;

	XXXsuzu_panel->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	rc = mipi_dsi_dcs_set_display_off(XXXsuzu_panel->dsi);
	if (rc < 0)
		dev_err(dev, "Cannot set display off: %d\n", rc);

	rc = mipi_dsi_dcs_enter_sleep_mode(XXXsuzu_panel->dsi);
	if (rc < 0)
		dev_err(dev, "Cannot enter sleep mode: %d\n", rc);

	msleep(100);

	return rc;
}

static int XXXsuzu_panel_unprepare(struct drm_panel *panel)
{
	struct XXXsuzu_panel *XXXsuzu_panel = to_XXXsuzu(panel);
	int rc = 0;

	if (!XXXsuzu_panel->prepared)
		return 0;
#ifdef MDSS_BUG_SOLVED
	if (XXXsuzu_panel->ts_reset_gpio) {
		gpiod_set_value(XXXsuzu_panel->ts_reset_gpio, 0);
		usleep_range(10000, 11000);
	}
#endif
	XXXsuzu_panel_off(XXXsuzu_panel);

	/* TODO: LAB/IBB */
#ifdef MDSS_BUG_SOLVED
	regulator_disable(XXXsuzu_panel->avdd_supply);
	regulator_disable(XXXsuzu_panel->vddio_supply);

	if (XXXsuzu_panel->pan_reset_gpio) {
		gpiod_set_value(XXXsuzu_panel->pan_reset_gpio, 0);
		usleep_range(10000, 11000);
	}

#endif
	XXXsuzu_panel->prepared = false;

	return rc;
}

static int XXXsuzu_panel_prepare(struct drm_panel *panel)
{
	struct XXXsuzu_panel *XXXsuzu_panel = to_XXXsuzu(panel);
	struct device *dev = &XXXsuzu_panel->dsi->dev;
	int rc;

	if (XXXsuzu_panel->prepared)
		return 0;

	/* Power rail VDDIO => in-cell panel main */
	rc = regulator_enable(XXXsuzu_panel->vddio_supply);
	if (rc < 0)
		return rc;

	msleep(80);

	/* Power rail AVDD => in-cell touch-controller main */
	rc = regulator_enable(XXXsuzu_panel->avdd_supply);
	if (rc < 0)
		dev_err(dev, "Cannot enable AVDD: %d\n", rc);
	else
		usleep_range(1000, 1100);

	/* TODO: LAB/IBB */

	/* Enable the in-cell supply to panel */
	rc = regulator_enable(XXXsuzu_panel->tvdd_supply);
	if (rc < 0) {
		dev_err(dev, "Cannot enable TVDD: %d\n", rc);
		goto poweroff_s1;
	} else {
		usleep_range(1000, 1100);
	}

	/* Enable the in-cell supply to touch-controller */
	rc = gpiod_direction_output(XXXsuzu_panel->ts_vddio_gpio, 0);
	if (rc) {
		dev_err(dev, "Cannot set tvddio-gpio direction: %d", rc);
		goto poweroff_s2;
	}
	usleep_range(1000, 1100);

	if (XXXsuzu_panel->ts_reset_gpio)
		gpiod_set_value(XXXsuzu_panel->ts_reset_gpio, 1);

#ifdef MDSS_BUG_SOLVED
	if (XXXsuzu_panel->pan_reset_gpio) {
		gpiod_set_value(XXXsuzu_panel->pan_reset_gpio, 0);
		usleep_range(10000, 10000);
		gpiod_set_value(XXXsuzu_panel->pan_reset_gpio, 1);
		usleep_range(10000, 11000);
	};
#endif

	rc = XXXsuzu_panel_init(XXXsuzu_panel);
	if (rc < 0) {
		dev_err(dev, "Cannot initialize panel: %d\n", rc);
		goto poweroff_s2;
	}

	rc = XXXsuzu_panel_on(XXXsuzu_panel);
	if (rc < 0) {
		dev_err(dev, "Cannot poweron panel: %d\n", rc);
		goto poweroff_s2;
	}

	XXXsuzu_panel->prepared = true;

	return 0;

poweroff_s2:
	/* Disable it to avoid current/voltage spikes in the enable path */
	regulator_disable(XXXsuzu_panel->tvdd_supply);
poweroff_s1:
	regulator_disable(XXXsuzu_panel->avdd_supply);
	regulator_disable(XXXsuzu_panel->vddio_supply);

	return rc;
}

static const struct drm_display_mode default_mode = {
	.clock = 149506,
	.hdisplay = 1080,
	.hsync_start = 1080 + 56,
	.hsync_end = 1080 + 56 + 8,
	.htotal = 1080 + 56 + 8 + 8,
	.vdisplay = 1920,
	.vsync_start = 1920 + 227,
	.vsync_end = 1920 + 227 + 8,
	.vtotal = 1920 + 227 + 8 + 8,
	.vrefresh = 60,
	//.flags = 0,
};

static int XXXsuzu_panel_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;
	struct XXXsuzu_panel *XXXsuzu_panel = to_XXXsuzu(panel);
	struct device *dev = &XXXsuzu_panel->dsi->dev;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = 61;
	panel->connector->display_info.height_mm = 110;

	return 1;
}

static const struct drm_panel_funcs XXXsuzu_panel_funcs = {
	.disable = XXXsuzu_panel_disable,
	.unprepare = XXXsuzu_panel_unprepare,
	.prepare = XXXsuzu_panel_prepare,
	.enable = XXXsuzu_panel_enable,
	.get_modes = XXXsuzu_panel_get_modes,
};

static const struct of_device_id XXXsuzu_of_match[] = {
	{ .compatible = "jdi,syn-incell,xxxsuzu", },
	{ }
};
MODULE_DEVICE_TABLE(of, XXXsuzu_of_match);

static int XXXsuzu_panel_add(struct XXXsuzu_panel *XXXsuzu_panel)
{
	struct device *dev = &XXXsuzu_panel->dsi->dev;
	int rc;

	XXXsuzu_panel->mode = &default_mode;

	XXXsuzu_panel->vddio_supply = devm_regulator_get(dev, "vddio");
	if (IS_ERR(XXXsuzu_panel->vddio_supply)) {
		dev_err(dev, "cannot get vddio regulator: %ld\n",
			PTR_ERR(XXXsuzu_panel->vddio_supply));
		return PTR_ERR(XXXsuzu_panel->vddio_supply);
	}

	XXXsuzu_panel->avdd_supply = devm_regulator_get_optional(dev, "avdd");
	if (IS_ERR(XXXsuzu_panel->avdd_supply)) {
		dev_err(dev, "cannot get avdd regulator: %ld\n",
			PTR_ERR(XXXsuzu_panel->avdd_supply));
		XXXsuzu_panel->avdd_supply = NULL;
	}

	XXXsuzu_panel->avdd_supply = devm_regulator_get_optional(dev, "tvdd");
	if (IS_ERR(XXXsuzu_panel->tvdd_supply)) {
		dev_err(dev, "cannot get tvdd regulator: %ld\n",
			PTR_ERR(XXXsuzu_panel->tvdd_supply));
		XXXsuzu_panel->avdd_supply = NULL;
	}

	XXXsuzu_panel->pan_reset_gpio = devm_gpiod_get(dev,
					"preset", GPIOD_ASIS);
	if (IS_ERR(XXXsuzu_panel->pan_reset_gpio)) {
		dev_err(dev, "cannot get preset-gpio: %ld\n",
			PTR_ERR(XXXsuzu_panel->pan_reset_gpio));
		XXXsuzu_panel->pan_reset_gpio = NULL;
	}

	XXXsuzu_panel->ts_vddio_gpio = devm_gpiod_get(dev,
					"tvddio", GPIOD_ASIS);
	if (IS_ERR(XXXsuzu_panel->ts_vddio_gpio)) {
		dev_err(dev, "cannot get tvddio-gpio: %ld\n",
			PTR_ERR(XXXsuzu_panel->ts_vddio_gpio));
		XXXsuzu_panel->ts_vddio_gpio = NULL;
	}

	XXXsuzu_panel->ts_reset_gpio = devm_gpiod_get(dev,
					"treset", GPIOD_ASIS);
	if (IS_ERR(XXXsuzu_panel->ts_reset_gpio)) {
		dev_err(dev, "cannot get treset-gpio: %ld\n",
			PTR_ERR(XXXsuzu_panel->ts_reset_gpio));
		XXXsuzu_panel->ts_reset_gpio = NULL;
	}

	drm_panel_init(&XXXsuzu_panel->base);
	XXXsuzu_panel->base.funcs = &XXXsuzu_panel_funcs;
	XXXsuzu_panel->base.dev = dev;

	rc = drm_panel_add(&XXXsuzu_panel->base);
	if (rc < 0)
		pr_err("drm panel add failed\n");

	return rc;
}

static void XXXsuzu_panel_del(struct XXXsuzu_panel *XXXsuzu_panel)
{
	if (XXXsuzu_panel->base.dev)
		drm_panel_remove(&XXXsuzu_panel->base);
}

static int XXXsuzu_panel_probe(struct mipi_dsi_device *dsi)
{
	struct XXXsuzu_panel *XXXsuzu_panel;
	int rc;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_CLOCK_NON_CONTINUOUS;

	XXXsuzu_panel = devm_kzalloc(&dsi->dev,
				sizeof(*XXXsuzu_panel), GFP_KERNEL);
	if (!XXXsuzu_panel)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, XXXsuzu_panel);
	XXXsuzu_panel->dsi = dsi;

	rc = XXXsuzu_panel_add(XXXsuzu_panel);
	if (rc < 0)
		return rc;

	return mipi_dsi_attach(dsi);
}

static int XXXsuzu_panel_remove(struct mipi_dsi_device *dsi)
{
	struct XXXsuzu_panel *XXXsuzu_panel = mipi_dsi_get_drvdata(dsi);
	struct device *dev = &XXXsuzu_panel->dsi->dev;
	int ret;

	ret = XXXsuzu_panel_disable(&XXXsuzu_panel->base);
	if (ret < 0)
		dev_err(dev, "failed to disable panel: %d\n", ret);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(dev, "Cannot detach from DSI host: %d\n", ret);

	XXXsuzu_panel_del(XXXsuzu_panel);

	return 0;
}

static void XXXsuzu_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct XXXsuzu_panel *XXXsuzu_panel = mipi_dsi_get_drvdata(dsi);

	XXXsuzu_panel_disable(&XXXsuzu_panel->base);
}

static struct mipi_dsi_driver XXXsuzu_panel_driver = {
	.driver = {
		.name = "panel-jdi-syn-XXXsuzu",
		.of_match_table = XXXsuzu_of_match,
	},
	.probe = XXXsuzu_panel_probe,
	.remove = XXXsuzu_panel_remove,
	.shutdown = XXXsuzu_panel_shutdown,
};
module_mipi_dsi_driver(XXXsuzu_panel_driver);

MODULE_AUTHOR("AngeloGioacchino Del Regno <kholk11@gmail.com>");
MODULE_DESCRIPTION("XXXsuzu LCD panel driver");
MODULE_LICENSE("GPL v2");
