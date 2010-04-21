/*
 * libudev-based hotplug backend for libusb
 * Copyright (C) 2009 Daniel Drake <dan@reactivated.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <libudev.h>

#include "libusbi.h"

struct udev_hp_priv {
	struct udev_monitor *mon;
};

static struct udev_hp_priv *get_priv(struct libusb_context *ctx)
{
	return (struct udev_hp_priv *) ctx->hp_priv;
}

static int op_init(struct libusb_context *ctx)
{
	struct udev_hp_priv *priv = malloc(sizeof(struct udev_hp_priv));
	if (!priv)
		return LIBUSB_ERROR_NO_MEM;

	ctx->hp_priv = priv;
	memset(priv, 0, sizeof(*priv));
	return 0;
}

static void op_exit(struct libusb_context *ctx)
{
	free(ctx->hp_priv);
}

static int op_enable(struct libusb_context *ctx, int coldplug)
{
	struct udev_hp_priv *priv = get_priv(ctx);
	struct udev *udev;
	int r = LIBUSB_ERROR_OTHER;
	int fd;

	udev = udev_new();
	if (!udev)
		return LIBUSB_ERROR_OTHER;

	priv->mon = udev_monitor_new_from_netlink(udev, "udev");
	if (!priv->mon)
		goto err_unref_udev;

	fd = udev_monitor_get_fd(priv->mon);
	r = usbi_hotplug_set_pollfd(ctx, fd, POLLIN);
	if (r < 0)
		goto err_unref_monitor;

	r = udev_monitor_filter_add_match_subsystem_devtype(priv->mon,
		"usb", "usb_device");
	if (r < 0)
		goto err_unref_monitor;

	r = udev_monitor_enable_receiving(priv->mon);
	if (r < 0) {
		r = LIBUSB_ERROR_OTHER;
		goto err_unref_monitor;
	}

	// FIXME coldplug using enumerate

	return 0;

err_unref_monitor:
	udev_monitor_unref(priv->mon);	
err_unref_udev:
	udev_unref(udev);
	return r;
}

static void op_disable(struct libusb_context *ctx)
{
	struct udev_hp_priv *priv = get_priv(ctx);
	struct udev *udev = udev_monitor_get_udev(priv->mon);

	/* FIXME: udev_monitor_disconnect is available but not in a header
	 * should we call it? */
	udev_monitor_unref(priv->mon);
	udev_unref(udev);
}

static int device_added(struct libusb_context *ctx, struct udev_device *udevice)
{
	struct libusb_device *dev;
	const char *_busnum = udev_device_get_sysattr_value(udevice, "busnum");
	const char *_devaddr = udev_device_get_sysattr_value(udevice,  "devnum");
	uint8_t busnum, devaddr;
	int r;

	if (!_busnum || !_devaddr) {
		usbi_err(ctx, "missing busnum/devnum??");
		return LIBUSB_ERROR_OTHER;
	}

	busnum = atoi(_busnum);
	devaddr = atoi(_devaddr);

	if (!busnum || !devaddr) {
		usbi_err(ctx, "zero busnum/devnum??");
		return LIBUSB_ERROR_OTHER;
	}

	r = usbi_linux_direct_get_device(ctx, busnum, devaddr,
		udev_device_get_syspath(udevice), &dev);
	if (r < 0)
		return r;

	usbi_hotplug_notify_connect(ctx, dev);

	/* drop reference if we were the cause of device allocation */
	if (r == 1)
		libusb_unref_device(dev);

	return 0;
}

static int op_handle_events(struct libusb_context *ctx)
{
	const char *action;
	struct udev_hp_priv *priv = get_priv(ctx);
	struct udev_device *device = udev_monitor_receive_device(priv->mon);
	int r = 0;

	if (!device)
		return 0;

	usbi_dbg("event for %s %s", udev_device_get_syspath(device), udev_device_get_devtype(device));
	action = udev_device_get_action(device);
	if (action && strcmp(action, "add") == 0)
		r = device_added(ctx, device);

	udev_device_unref(device);
	return r;
}

const struct usbi_hp_backend usbi_hp_backend_libudev = {
	.init = op_init,
	.exit = op_exit,
	.enable = op_enable,
	.disable = op_disable,
	.handle_events = op_handle_events,
};

