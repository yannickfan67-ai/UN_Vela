#ifndef UN_VELA_IMAGE_H
#define UN_VELA_IMAGE_H
#include <stddef.h>
#include <stdint.h>

typedef struct VelaImageInfo {
    uint32_t width;
    uint32_t height;
    uint8_t channels;
} VelaImageInfo;

/* Lightweight built-in decoder for uncompressed 24/32-bit BMP and PPM P6. */
int vela_image_probe(const uint8_t *data,size_t len,VelaImageInfo *info);
int vela_image_decode_rgb24(const uint8_t *data,size_t len,uint8_t *rgb,size_t rgb_cap,VelaImageInfo *info);

#endif
