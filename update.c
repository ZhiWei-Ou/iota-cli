/**
 * @brief 
 * @file update.c
 * @author Oswin
 * @date 2025-12-26
 * @details
 */
#include "os_file.h"
#include "xlog.h"
#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>

const uint8_t magic[] = { 'I', 'O', 'T', 'A' };

extern char *update_image;
extern xbool_t update_skip_checksum;
extern xbool_t update_reboot;

#pragma pack(push, 1)
typedef struct {
    uint8_t magic[4];
    char datetime[20];
    uint32_t size;
} image_header_t;
#pragma pack(pop)

static err_t stream_decrypt_gcm(FILE *in_fp, FILE *out_fp);

int update_feature_entry() {
    if (!update_image) {
        XLOG_E("No update image specified.");
        return X_RET_INVAL;
    }

    FILE *fp = os_file_open(update_image, "rb");
    if (!fp) {
        XLOG_E("Failed to open update image: %s", update_image);
        return X_RET_ERROR;
    }

    err_t err = X_RET_OK;
    image_header_t header = {0};

    size_t header_read_size = fread(&header, 1, sizeof(image_header_t), fp);
    if (header_read_size != sizeof(image_header_t)) {
        XLOG_E("Failed to read image header.");
        os_file_close(fp);
        return X_RET_ERROR;
    }

    if (memcmp(header.magic, magic, sizeof(magic)) != 0) {
        XLOG_E("Invalid image magic.");
        os_file_close(fp);
        return X_RET_BADFMT;
    }

    XLOG_I("Image datetime: %.20s", header.datetime);
    XLOG_I("Image size: %u bytes", header.size);



    os_file_close(fp);
    return X_RET_OK;
}
