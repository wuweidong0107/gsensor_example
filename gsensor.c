#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

#include "gsensor.h"
#include "inputdev.h"

#define GSENSOR_IOCTL_MAGIC			        'a'
#define GSENSOR_IOCTL_INIT					_IO(GSENSOR_IOCTL_MAGIC, 0x01)
#define GSENSOR_IOCTL_RESET					_IO(GSENSOR_IOCTL_MAGIC, 0x04)
#define GSENSOR_IOCTL_CLOSE					_IO(GSENSOR_IOCTL_MAGIC, 0x02)
#define GSENSOR_IOCTL_START					_IO(GSENSOR_IOCTL_MAGIC, 0x03)
#define GSENSOR_IOCTL_GETDATA				_IOR(GSENSOR_IOCTL_MAGIC, 0x08, char[GBUFF_SIZE+1])
#define GSENSOR_IOCTL_APP_SET_RATE			_IOW(GSENSOR_IOCTL_MAGIC, 0x10, short)
#define GSENSOR_IOCTL_GET_CALIBRATION		_IOR(GSENSOR_IOCTL_MAGIC, 0x11, int[3])

struct gsensor_handle {
    int char_fd;
    input_t *in;

    struct {
        int c_errno;
        char errmsg[96];
    } error;
};

static int _gsensor_error(gsensor_t* ls, int code, int c_errno, const char* fmt, ...)
{
    va_list ap;
    ls->error.c_errno = c_errno;
    va_start(ap, fmt);
    vsnprintf(ls->error.errmsg, sizeof(ls->error.errmsg), fmt, ap);
    va_end(ap);

    if (c_errno) {
        char buf[64];
        strerror_r(c_errno, buf, sizeof(buf));
        snprintf(ls->error.errmsg + strlen(ls->error.errmsg), 
            sizeof(ls->error.errmsg) - strlen(ls->error.errmsg),
            ": %s [errno %d]", buf, c_errno);
    }
    return code;
}

int gsensor_errno(gsensor_t* ls)
{
    return ls->error.c_errno;
}

const char* gsensor_errmsg(gsensor_t* ls)
{
    return ls->error.errmsg;
}

gsensor_t* gsensor_new(void)
{
    gsensor_t* ls = calloc(1, sizeof(gsensor_t));
    if (ls == NULL)
        return NULL;

    ls->in = input_new();
    if (!ls->in)
        goto fail;

    return ls;
fail:
    gsensor_free(ls);
    return NULL;
}

void gsensor_free(gsensor_t* ls)
{
    if (ls->in)
        free(ls->in);
    if (ls)
        free(ls);
}

int gsensor_open(gsensor_t* ls, const char* path, const char* input_path)
{
    if ((ls->char_fd = open(path, O_RDWR)) < 0)
        return _gsensor_error(ls, GSENSOR_ERROR_OPEN, errno, "Opening gsensor chardev\"%s\"", path);

    if (input_open(ls->in, input_path) !=0) {
        return _gsensor_error(ls, GSENSOR_ERROR_OPEN, 0, "Openning gsensor inputdev %s", input_path);
    }

    return 0;
}

int gsensor_close(gsensor_t* ls)
{
    if (ls->char_fd > 0) {
        if (close(ls->char_fd) < 0)
            return _gsensor_error(ls, GSENSOR_ERROR_CLOSE, errno, "Closing gsensor/misc");
        ls->char_fd = -1;
    }
    if (ls->in) {
        input_close(ls->in);
    }
    return 0;
}

int gsensor_set_enable(gsensor_t* ls, unsigned int value)
{
    if (value > 0) {
        if (ioctl(ls->char_fd, GSENSOR_IOCTL_START) < 0) {
            return _gsensor_error(ls, GSENSOR_ERROR_CONFIGURE, errno, "Setting gsensor attributes");
        }
    } else {
        if (ioctl(ls->char_fd, GSENSOR_IOCTL_CLOSE) < 0) {
            return _gsensor_error(ls, GSENSOR_ERROR_CONFIGURE, errno, "Setting gsensor attributes");
        }
    }
    return 0;
}

int gsensor_read(gsensor_t* ls,  struct input_event ev[], size_t ev_len, int timeout_ms)
{
    int ev_read = input_read(ls->in, ev, ev_len, timeout_ms);
    if (ev_read < 0) {
        return _gsensor_error(ls, GSENSOR_ERROR_IO, errno, "Reading gsensor");
    }
    return ev_read;
}