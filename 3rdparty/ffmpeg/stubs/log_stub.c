#include "libavformat/avformat.h"

const char *av_default_item_name(void *ptr) {
    return (*(AVClass **) ptr)->class_name;
}

void av_log(void* avcl, int level, const char *fmt, ...) {
}

int av_log_get_level(void) {
    return AV_LOG_QUIET;
}

void av_log_set_flags(int arg) {
}

void av_log_set_level(int level) {
}

void av_hex_dump_log(void *avcl, int level, const uint8_t *buf, int size) {
}

void avpriv_request_sample(void *avc, const char *msg, ...) {
}

void avpriv_report_missing_feature(void *avc, const char *msg, ...) {
}