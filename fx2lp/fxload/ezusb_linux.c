/*
 * Copyright (c) 2010 Linux Hotplug Project
 * 
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ident "$Id$"
# ifdef __linux__

# include  <assert.h>
# include  <stdio.h>
# include  <stdlib.h>
# include  <errno.h>
# include  <limits.h>
# include  <fcntl.h>
# include  <unistd.h>

# include  <sys/ioctl.h>

# include  <linux/version.h>
# include  <linux/usb/ch9.h>
# include  <linux/usbdevice_fs.h>

# include  <linux/version.h>
# include  <linux/usb/ch9.h>
# include  <linux/usbdevice_fs.h>

# include "ezusb.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,3)
/*
 * in 2.5, "struct usbdevfs_ctrltransfer" fields were renamed
 * to match the USB spec
 */
#	define bRequestType	requesttype
#	define bRequest		request
#	define wValue		value
#	define wIndex		index
#	define wLength		length
#endif

/*
 * Issue a control request to the specified device.
 * This is O/S specific ...
 */
int ctrl_msg_linux (
    struct ezusb_backend		*backend,
    unsigned char			requestType,
    unsigned char			request,
    unsigned short			value,
    unsigned short			index,
    unsigned char			*data,
    size_t				length
) {
    struct usbdevfs_ctrltransfer	ctrl;
    int device;

    assert(backend != NULL && "internal error: no backend");
    assert(backend->priv_data != NULL && "priv_data empty!");

    device = *((int *)backend->priv_data);

    /*
     * Shouldn't happen, but just in case check if we have the right
     * descriptor
     */
    if (device < 0) {
        logerror("wrong filedescriptor");
	return -EINVAL;
    }

    if (length > USHRT_MAX) {
	logerror("length too big\n");
	return -EINVAL;
    }

    /*
     * Match system-specific calls.
     */
    assert((requestType == EZUSB_INPUT || requestType == EZUSB_OUTPUT) &&
	"internal error: wrong request type");
    if (requestType == EZUSB_INPUT) {
	requestType = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
    } else if (requestType == EZUSB_OUTPUT) {
        requestType = USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
    }

    /* 8 bytes SETUP */
    ctrl.bRequestType = requestType;
    ctrl.bRequest = request;
    ctrl.wValue   = value;
    ctrl.wLength  = (unsigned short) length;
    ctrl.wIndex = index;

    /* "length" bytes DATA */
    ctrl.data = data;

    ctrl.timeout = 10000;

    return ioctl (device, USBDEVFS_CONTROL, &ctrl);
}

int ezusb_linux_open(struct ezusb_backend *be, const char *device, int mode)
{
      int fd;

      fd = open(device, mode);
      if (fd == -1)
          return -1;

      be->priv_data = calloc(1, sizeof(fd));
      assert(be->priv_data != NULL);
      *((int *)be->priv_data) = fd;

      return 0;
}

int ezusb_linux_close(struct ezusb_backend *be)
{
      int fd;

      assert(be != NULL);
      if (be->priv_data) {
	  fd = *((int *)be->priv_data);
	  free(be->priv_data);
	  be->priv_data = NULL;
	  return close(fd);
      }
      return 0;
}

struct ezusb_backend ezusb_backend_linux = {
      .name = "linux",
      .ctrl_msg = ctrl_msg_linux,
      .priv_data = NULL,
      .open = ezusb_linux_open,
      .close = ezusb_linux_close
};
#endif /* __linux__ */
