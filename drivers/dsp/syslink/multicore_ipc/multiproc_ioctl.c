/*
*  multiproc_ioctl.c
*
*  This provides the ioctl interface for multiproc module
*
*  Copyright (C) 2008-2009 Texas Instruments, Inc.
*
*  This package is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*  THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
*  WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
*  PURPOSE.
*/
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <multiproc.h>
#include <multiproc_ioctl.h>

/*
 * ======== mproc_ioctl_setup ========
 *  Purpose:
 *  This wrapper function will call the multproc function
 *  to setup the module
 */
static int mproc_ioctl_setup(struct multiproc_cmd_args *cargs)
{
	struct multiproc_config config;
	s32 status = 0;
	ulong size;

	size = copy_from_user(&config,
				cargs->args.setup.config,
				sizeof(struct multiproc_config));
	if (size) {
		status = -EFAULT;
		goto exit;
	}

	cargs->api_status = multiproc_setup(&config);

exit:
	return status;
}

/*
 * ======== mproc_ioctl_destroy ========
 *	Purpose:
 *	This wrapper function will call the multproc function
 *	to destroy the module
 */
static int mproc_ioctl_destroy(struct multiproc_cmd_args *cargs)
{
	cargs->api_status = multiproc_destroy();
	return 0;
}

/*
 * ======== mproc_ioctl_get_config ========
 *	Purpose:
 *	This wrapper function will call the multproc function
 *	to get the default configuration the module
 */
static int mproc_ioctl_get_config(struct multiproc_cmd_args *cargs)
{
	struct multiproc_config config;
	u32 size;

	multiproc_get_config(&config);
	size = copy_to_user(cargs->args.get_config.config, &config,
				sizeof(struct multiproc_config));
	if (size) {
		cargs->api_status = -EFAULT;
		return 0;
	}
	cargs->api_status = 0;
	return 0;
}

/*
 * ======== mproc_ioctl_setup ========
 *	Purpose:
 *	This wrapper function will call the multproc function
 *	to setup the module
 */
static int multiproc_ioctl_set_local_id(struct multiproc_cmd_args *cargs)
{
	cargs->api_status = multiproc_set_local_id(cargs->args.set_local_id.id);
	return 0;
}

/*
 * ======== multiproc_ioctl ========
 *  Purpose:
 *  This ioctl interface for multiproc module
 */
int multiproc_ioctl(struct inode *inode, struct file *filp,
			unsigned int cmd, unsigned long args)
{
	s32 status = 0;
	s32 size = 0;
	struct multiproc_cmd_args __user *uarg =
				(struct multiproc_cmd_args __user *)args;
	struct multiproc_cmd_args cargs;


	if (_IOC_DIR(cmd) & _IOC_READ)
		status = !access_ok(VERIFY_WRITE, uarg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		status = !access_ok(VERIFY_READ, uarg, _IOC_SIZE(cmd));

	if (status) {
		status = -EFAULT;
		goto exit;
	}

	/* Copy the full args from user-side */
	size = copy_from_user(&cargs, uarg,
					sizeof(struct multiproc_cmd_args));
	if (size) {
		status = -EFAULT;
		goto exit;
	}

	switch (cmd) {
	case CMD_MULTIPROC_SETUP:
		status = mproc_ioctl_setup(&cargs);
		break;

	case CMD_MULTIPROC_DESTROY:
		status = mproc_ioctl_destroy(&cargs);
		break;

	case CMD_MULTIPROC_GETCONFIG:
		status = mproc_ioctl_get_config(&cargs);
		break;

	case CMD_MULTIPROC_SETLOCALID:
		status = multiproc_ioctl_set_local_id(&cargs);
		break;

	default:
		WARN_ON(cmd);
		status = -ENOTTY;
		break;
	}

	/* Copy the full args to the user-side. */
	size = copy_to_user(uarg, &cargs, sizeof(struct multiproc_cmd_args));
	if (size) {
		status = -EFAULT;
		goto exit;
	}

exit:
	return status;

}

