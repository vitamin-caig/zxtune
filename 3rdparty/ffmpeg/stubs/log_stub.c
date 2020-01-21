#include "libavutil/log.h"

const char *av_default_item_name(void *ptr) {
    return (*(AVClass **) ptr)->class_name;
}

void av_log(void* avcl, int level, const char *fmt, ...) {
}

void avpriv_request_sample(void *avc, const char *msg, ...) {
}
