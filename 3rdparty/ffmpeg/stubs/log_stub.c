#include "libavutil/log.h"

const char *av_default_item_name(void *ptr) {
    return (*(AVClass **) ptr)->class_name;
}

void av_log(void* avcl, int level, const char *fmt, ...) {
}

int av_log_get_level(void) {
    return AV_LOG_QUIET;
}

void avpriv_request_sample(void *avc, const char *msg, ...) {
}

void avpriv_report_missing_feature(void *avc, const char *msg, ...) {
}