/*
 * linux/drivers/video/omap2/dss/dpi.c
 *
 * Copyright (C) 2009 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
 *
 * Some code and ideas taken from drivers/video/omap/ driver
 * by Imre Deak.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define DSS_SUBSYS_NAME "DPI"

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include <plat/display.h>
#include <plat/cpu.h>

#include "dss.h"

static struct {
	int update_enabled;
} dpi;

#ifndef CONFIG_ARCH_OMAP4
/*TODO: OMAP4: check the clock divisor mechanism? */
#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL
static int dpi_set_dsi_clk(bool is_tft, unsigned long pck_req,
		unsigned long *fck, int *lck_div, int *pck_div)
{
	struct dsi_clock_info dsi_cinfo;
	struct dispc_clock_info dispc_cinfo;
	int r;

	r = dsi_pll_calc_clock_div_pck(dsi1, is_tft, pck_req, &dsi_cinfo,
			&dispc_cinfo);
	if (r)
		return r;

	r = dsi_pll_set_clock_div(dsi1, &dsi_cinfo);
	if (r)
		return r;

	dss_select_clk_source(0, 1);

	r = dispc_set_clock_div(&dispc_cinfo);
	if (r)
		return r;

	*fck = dsi_cinfo.dsi1_pll_fclk;
	*lck_div = dispc_cinfo.lck_div;
	*pck_div = dispc_cinfo.pck_div;

	return 0;
}
#else
static int dpi_set_dispc_clk(bool is_tft, unsigned long pck_req,
		unsigned long *fck, int *lck_div, int *pck_div)
{
	struct dss_clock_info dss_cinfo;
	struct dispc_clock_info dispc_cinfo;
	int r;

#ifdef CONFIG_ARCH_OMAP4
/*OMAP4: check this later?*/
	return 0;
#endif

	r = dss_calc_clock_div(is_tft, pck_req, &dss_cinfo, &dispc_cinfo);
	if (r)
		return r;

	r = dss_set_clock_div(&dss_cinfo);
	if (r)
		return r;

	r = dispc_set_clock_div(&dispc_cinfo);
	if (r)
		return r;

	*fck = dss_cinfo.fck;
	*lck_div = dispc_cinfo.lck_div;
	*pck_div = dispc_cinfo.pck_div;

	return 0;
}
#endif
#endif /*if 0*/

static int dpi_set_mode(struct omap_dss_device *dssdev)
{
	struct omap_video_timings *t = &dssdev->panel.timings;
	int lck_div, pck_div;
	unsigned long fck;
	unsigned long pck;
	bool is_tft;
	int r = 0;
#ifndef CONFIG_ARCH_OMAP4
	dss_clk_enable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif
	/* TODO: add LCD2 support here*/
	dispc_set_pol_freq(DEFAULT_CHANNEL, dssdev->panel.config,
			dssdev->panel.acbi, dssdev->panel.acb);

#ifndef CONFIG_ARCH_OMAP4
	is_tft = (dssdev->panel.config & OMAP_DSS_LCD_TFT) != 0;

#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL
	r = dpi_set_dsi_clk(is_tft, t->pixel_clock * 1000,
			&fck, &lck_div, &pck_div);
#else
	r = dpi_set_dispc_clk(is_tft, t->pixel_clock * 1000,
			&fck, &lck_div, &pck_div);
#endif
	if (r)
		goto err0;

#ifndef CONFIG_ARCH_OMAP4
	pck = fck / lck_div / pck_div / 1000;
#else
	pck = 0;
#endif

	if (pck != t->pixel_clock) {
		DSSWARN("Could not find exact pixel clock. "
				"Requested %d kHz, got %lu kHz\n",
				t->pixel_clock, pck);

		t->pixel_clock = pck;
	}

#endif
	/* TODO: OMAP4: add LCD 2 support here*/
	dispc_set_lcd_timings(DEFAULT_CHANNEL, t);
#ifndef CONFIG_ARCH_OMAP4
err0:
	dss_clk_disable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif
	return r;
}

static int dpi_basic_init(struct omap_dss_device *dssdev)
{
	bool is_tft;

	is_tft = (dssdev->panel.config & OMAP_DSS_LCD_TFT) != 0;

	dispc_set_parallel_interface_mode(DEFAULT_CHANNEL,
					OMAP_DSS_PARALLELMODE_BYPASS);

	/*TODO: change here for LCD 2 support */
	dispc_set_lcd_display_type(DEFAULT_CHANNEL,
		is_tft ? OMAP_DSS_LCD_DISPLAY_TFT : OMAP_DSS_LCD_DISPLAY_STN);
	dispc_set_tft_data_lines(DEFAULT_CHANNEL,
					dssdev->phy.dpi.data_lines);

	return 0;
}

static int dpi_display_enable(struct omap_dss_device *dssdev)
{
	int r;

	r = omap_dss_start_device(dssdev);
	if (r) {
		DSSERR("failed to start device\n");
		goto err0;
	}

	if (dssdev->state != OMAP_DSS_DISPLAY_DISABLED) {
		DSSERR("display already enabled\n");
		r = -EINVAL;
		goto err1;
	}

#ifndef CONFIG_ARCH_OMAP4
	dss_clk_enable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif

	r = dpi_basic_init(dssdev);
	if (r)
		goto err2;
#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL
	dss_clk_enable(DSS_CLK_FCK2);

	if (cpu_is_omap3630())
		r = dsi_pll_init(dsi1, dssdev, 1, 1);
	else
		r = dsi_pll_init(dsi1, dssdev, 0, 1);
	if (r)
		goto err3;
#elif defined(CONFIG_ARCH_OMAP4)
	BUG();
#endif  /* #if 0 */
	r = dpi_set_mode(dssdev);
	if (r)
		goto err4;

	mdelay(2);

	/* TODO: change here if LCD2 support is needed */
	dispc_enable_lcd_out(DEFAULT_CHANNEL, 1);

	r = dssdev->driver->enable(dssdev);
	if (r)
		goto err5;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;

err5:
	dispc_enable_lcd_out(DEFAULT_CHANNEL, 0);
err4:
#ifndef CONFIG_ARCH_OMAP4
#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL
	dsi_pll_uninit(dsi1);
#endif
err3:
#ifndef CONFIG_ARCH_OMAP4
	dss_clk_disable(DSS_CLK_FCK2);
#endif
#endif
err2:
#ifndef CONFIG_ARCH_OMAP4
	dss_clk_disable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif
err1:
	omap_dss_stop_device(dssdev);
err0:
	return r;
}

static int dpi_display_resume(struct omap_dss_device *dssdev);

static void dpi_display_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->state == OMAP_DSS_DISPLAY_DISABLED)
		return;

	if (dssdev->state == OMAP_DSS_DISPLAY_SUSPENDED)
		dpi_display_resume(dssdev);

	dssdev->driver->disable(dssdev);

	/* TODO: change here if LCD2 support is needed */
	dispc_enable_lcd_out(DEFAULT_CHANNEL, 0);

#ifndef CONFIG_ARCH_OMAP4
#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL
	dss_select_clk_source(0, 0);
	dsi_pll_uninit(dsi1);
	dss_clk_disable(DSS_CLK_FCK2);
#endif

	dss_clk_disable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif
	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;

	omap_dss_stop_device(dssdev);
}

static int dpi_display_suspend(struct omap_dss_device *dssdev)
{
	if (dssdev->state != OMAP_DSS_DISPLAY_ACTIVE)
		return -EINVAL;

	DSSDBG("dpi_display_suspend\n");

	if (dssdev->driver->suspend)
		dssdev->driver->suspend(dssdev);

	/* TODO: change here if LCD2 support is needed */
	dispc_enable_lcd_out(DEFAULT_CHANNEL, 0);
#ifndef CONFIG_ARCH_OMAP4
	dss_clk_disable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;

	return 0;
}

static int dpi_display_resume(struct omap_dss_device *dssdev)
{
	if (dssdev->state != OMAP_DSS_DISPLAY_SUSPENDED)
		return -EINVAL;

	DSSDBG("dpi_display_resume\n");

#ifndef CONFIG_ARCH_OMAP4
	dss_clk_enable(DSS_CLK_ICK | DSS_CLK_FCK1);
#endif

	/* TODO: change here if LCD2 support is needed */
	dispc_enable_lcd_out(DEFAULT_CHANNEL, 1);

	if (dssdev->driver->resume)
		dssdev->driver->resume(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void dpi_set_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	DSSDBG("dpi_set_timings\n");
	dssdev->panel.timings = *timings;
	if (dssdev->state == OMAP_DSS_DISPLAY_ACTIVE) {
		dpi_set_mode(dssdev);
		dispc_go(DEFAULT_CHANNEL);
	}
}

static int dpi_check_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	bool is_tft;
	int r = 0;
	int lck_div, pck_div;
	unsigned long fck;
	unsigned long pck;

	if (!dispc_lcd_timings_ok(timings))
		return -EINVAL;

	if (timings->pixel_clock == 0)
		return -EINVAL;

	is_tft = (dssdev->panel.config & OMAP_DSS_LCD_TFT) != 0;

/*TODO: OMAP4: check the clock divisor mechanism? */
#ifdef CONFIG_OMAP2_DSS_USE_DSI_PLL
	{
		struct dsi_clock_info dsi_cinfo;
		struct dispc_clock_info dispc_cinfo;
		r = dsi_pll_calc_clock_div_pck(dsi1, is_tft,
				timings->pixel_clock * 1000,
				&dsi_cinfo, &dispc_cinfo);

		if (r)
			return r;

		fck = dsi_cinfo.dsi1_pll_fclk;
		lck_div = dispc_cinfo.lck_div;
		pck_div = dispc_cinfo.pck_div;
	}
#else
	{
		struct dss_clock_info dss_cinfo;
		struct dispc_clock_info dispc_cinfo;
		r = dss_calc_clock_div(is_tft, timings->pixel_clock * 1000,
				&dss_cinfo, &dispc_cinfo);

		if (r)
			return r;

		fck = dss_cinfo.fck;
		lck_div = dispc_cinfo.lck_div;
		pck_div = dispc_cinfo.pck_div;
	}
#endif

	pck = fck / lck_div / pck_div / 1000;

	timings->pixel_clock = pck;

	return 0;
}

static void dpi_get_timings(struct omap_dss_device *dssdev,
			struct omap_video_timings *timings)
{
	*timings = dssdev->panel.timings;
}

static int dpi_display_set_update_mode(struct omap_dss_device *dssdev,
		enum omap_dss_update_mode mode)
{
	if (mode == OMAP_DSS_UPDATE_MANUAL)
		return -EINVAL;

	if (mode == OMAP_DSS_UPDATE_DISABLED) {
		/* TODO: change here if LCD2 support is needed */
		dispc_enable_lcd_out(DEFAULT_CHANNEL, 0);
		dpi.update_enabled = 0;
	} else {
		/* TODO: change here if LCD2 support is needed */
		dispc_enable_lcd_out(DEFAULT_CHANNEL, 1);
		dpi.update_enabled = 1;
	}

	return 0;
}

static enum omap_dss_update_mode dpi_display_get_update_mode(
		struct omap_dss_device *dssdev)
{
	return dpi.update_enabled ? OMAP_DSS_UPDATE_AUTO :
		OMAP_DSS_UPDATE_DISABLED;
}

int dpi_init_display(struct omap_dss_device *dssdev)
{
	DSSDBG("init_display\n");

	dssdev->enable = dpi_display_enable;
	dssdev->disable = dpi_display_disable;
	dssdev->suspend = dpi_display_suspend;
	dssdev->resume = dpi_display_resume;
	dssdev->set_timings = dpi_set_timings;
	dssdev->check_timings = dpi_check_timings;
	dssdev->get_timings = dpi_get_timings;
	dssdev->set_update_mode = dpi_display_set_update_mode;
	dssdev->get_update_mode = dpi_display_get_update_mode;

	return 0;
}

int dpi_init(void)
{
	return 0;
}

void dpi_exit(void)
{
}

