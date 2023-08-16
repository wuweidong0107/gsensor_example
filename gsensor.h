#ifndef _GSENSOR_H
#define _GSENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/input.h>
enum gsensor_error_code {
    GSENSOR_ERROR_ARG = -1,
    GSENSOR_ERROR_OPEN = -2,
    GSENSOR_ERROR_QUERY = -3,
    GSENSOR_ERROR_CONFIGURE = -4,
    GSENSOR_ERROR_IO = -5,
    GSENSOR_ERROR_CLOSE = -6,
};

typedef struct gsensor_handle gsensor_t;

gsensor_t* gsensor_new(void);
void gsensor_free(gsensor_t* ls);
int gsensor_open(gsensor_t* ls, const char* path, const char* input_path);
int gsensor_close(gsensor_t* ls);
const char* gsensor_errmsg(gsensor_t* ls);
int gsensor_set_enable(gsensor_t* ls, unsigned int value);
int gsensor_read(gsensor_t* ls,  struct input_event ev[], size_t ev_len, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif