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

#define AES_GCM_KEY_LEN  ( 16 )
#define AES_GCM_IV_LEN  ( 12 )
#define AES_GCM_TAG_LEN ( 16 )
#define RSA_SIGNATURE_LEN ( 256 )

const uint8_t magic[] = { 'I', 'O', 'T', 'A' };
const uint8_t key[AES_GCM_KEY_LEN] = {0XE9, 0X29, 0X95, 0XAA, 0X05, 0XBD, 0XF2, 0X89, 0XC4, 0X71, 0XDC, 0X7F, 0X5C, 0X13, 0X34, 0XCD};

extern char *update_image;
extern xbool_t update_skip_auth_tag;
extern xbool_t update_reboot;
extern int stream_count;

#pragma pack(push, 1)
typedef struct {
    uint8_t magic[4];
    char datetime[20];
    uint32_t size;
    uint8_t iv[AES_GCM_IV_LEN];
    uint8_t signature[RSA_SIGNATURE_LEN];
    uint8_t reserved[12];
} image_header_t;
#pragma pack(pop)

// const size_t header_offset =sizeof(image_header_t);

static err_t stream_decrypt_gcm(FILE *in_fp,
                                FILE *out_fp,
                                const uint8_t key[AES_GCM_KEY_LEN],
                                const uint8_t iv[AES_GCM_IV_LEN],
                                size_t data_size,
                                const uint8_t tag[AES_GCM_TAG_LEN],
                                int stream_count,
                                xbool_t skip_auth_tag);

int update_feature_entry() {
    if (!update_image) {
        XLOG_E("No update image specified.");
        return X_RET_INVAL;
    }

    FILE *in = os_file_open(update_image, "rb");
    if (!in) {
        XLOG_E("Failed to open update image: %s", update_image);
        return X_RET_ERROR;
    }

    err_t err = X_RET_OK;
    image_header_t header = {0};
    uint8_t tag[16] = {0};

    // Read IOTA header
    size_t header_read_size = fread(&header, 1, sizeof(image_header_t), in);
    if (header_read_size != sizeof(image_header_t)) {
        XLOG_E("Failed to read image header.");
        os_file_close(in);
        return X_RET_ERROR;
    }

    // Read IOTA AES-GCM tag
    fseek(in, -AES_GCM_TAG_LEN, SEEK_END);
    size_t tag_read_size = fread(tag, 1, sizeof(tag), in);
    if (tag_read_size != sizeof(tag)) {
        XLOG_E("Failed to read image tag.");
        os_file_close(in);
        return X_RET_ERROR;
    }
    // XLOG_HEX_DUMP("Image tag:", tag, sizeof(tag));

    // Validate magic
    if (memcmp(header.magic, magic, sizeof(magic)) != 0) {
        XLOG_E("Invalid image magic.");
        os_file_close(in);
        return X_RET_BADFMT;
    }

    XLOG_D("Image datetime: %.20s", header.datetime);
    XLOG_D("Image size: %u bytes", header.size);
    XLOG_D("Image signature: (not displayed)");
    XLOG_I("Image header read successfully.");

    // Seek to the start of encrypted data
    fseek(in, sizeof(image_header_t), SEEK_SET);
    FILE *out = os_file_open("firmware", "wb");
    if (!out) {
        XLOG_E("Failed to open output firmware file.");
        os_file_close(in);
        return X_RET_ERROR;
    }

    err = stream_decrypt_gcm(in, out, key, header.iv, (size_t)header.size, tag,
                             stream_count, update_skip_auth_tag);
    if (err != X_RET_OK) {
        os_file_close(in);
        os_file_close(out);
        return err;
    }

    os_file_close(out);
    os_file_close(in);
    return X_RET_OK;
}

static err_t stream_decrypt_gcm(FILE *in_fp,
                                FILE *out_fp,
                                const uint8_t key[AES_GCM_KEY_LEN],
                                const uint8_t iv[AES_GCM_IV_LEN],
                                size_t data_size,
                                const uint8_t tag[AES_GCM_TAG_LEN],
                                int stream_count, 
                                xbool_t skip_auth_tag) {
    if (!in_fp || !out_fp) {
        return X_RET_INVAL;
    }

    err_t err = X_RET_OK;
    EVP_CIPHER_CTX *ctx = NULL;
    size_t processed_size = 0;
    size_t total_size = data_size;

    if(!(ctx = EVP_CIPHER_CTX_new())) {
        XLOG_E("EVP_CIPHER_CTX_new failed. error: %s", ERR_reason_error_string(ERR_get_error()));
        return X_RET_ERROR;
    }

    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL)) {
        XLOG_E("EVP_DecryptInit_ex failed. error: %s", ERR_reason_error_string(ERR_get_error()));
        return X_RET_ERROR;
    }

    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_LEN, NULL)) {
        XLOG_E("EVP_CIPHER_CTX_ctrl failed. error: %s", ERR_reason_error_string(ERR_get_error()));
        return X_RET_ERROR;
    }

    if(1 != EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))  {
        XLOG_E("EVP_DecryptInit_ex failed. error: %s", ERR_reason_error_string(ERR_get_error()));
        return X_RET_ERROR;
    }

    int len = 0;
    uint8_t *inbuf = malloc(stream_count);
    uint8_t *outbuf = malloc(stream_count);
    if (!inbuf) {
        XLOG_E("Failed to allocate input buffer.");
        EVP_CIPHER_CTX_free(ctx);
        return X_RET_NOMEM;
    }

    while (processed_size < total_size) {
        size_t remaining_size = total_size - processed_size;
        size_t to_read = xMIN(remaining_size, stream_count);
        size_t read_bytes = fread(inbuf, 1, to_read, in_fp);

        XLOG_D("Decrypting... %zu/%zu bytes processed (%f %%).",
               processed_size + read_bytes, total_size, ((double)(processed_size + read_bytes) / total_size) * 100.0);
        if (read_bytes != to_read) {
            XLOG_E("Failed to read encrypted data. Expected %zu bytes, got %zu bytes.", to_read, read_bytes);
            free(inbuf);
            EVP_CIPHER_CTX_free(ctx);
            return X_RET_ERROR;
        }

        if(1 != EVP_DecryptUpdate(ctx, outbuf, &len, inbuf, read_bytes)) {
            XLOG_E("EVP_DecryptUpdate failed. error: %s", ERR_reason_error_string(ERR_get_error()));
            free(inbuf);
            free(outbuf);
            EVP_CIPHER_CTX_free(ctx);
            return X_RET_ERROR;
        }

        if (len > 0) {
            if (fwrite(outbuf, 1, len, out_fp) != (size_t)len) {
                XLOG_E("Failed to write decrypted data.");
                free(inbuf);
                free(outbuf);
                EVP_CIPHER_CTX_free(ctx);
                return X_RET_ERROR;
            }
        }

        processed_size += read_bytes;
    }


    if (skip_auth_tag) {
        XLOG_W("Skipping authentication tag verification as per user request.");
        err = X_RET_OK; // Success for main return
    } else {
        /* Set expected tag value. */
        if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_LEN, (void *)tag)) {
            XLOG_E("EVP_CIPHER_CTX_ctrl failed. error: %s", ERR_reason_error_string(ERR_get_error()));
            free(inbuf);
            free(outbuf);
            EVP_CIPHER_CTX_free(ctx);
            return X_RET_ERROR;
        }

        /* Finalise the decryption. A positive return value indicates success,
         * anything else is a failure - the plaintext is not trustworthy.
         */
        err = EVP_DecryptFinal_ex(ctx, outbuf, &len);

        if(err > 0) {
            /* Success */
            if (len > 0) {
                fwrite(outbuf, 1, len, out_fp);
            }
            XLOG_I("Decrypted %ld bytes successfully.", processed_size);
            err = X_RET_OK; // Success for main return
        } else {
            /* Verify failed */
            XLOG_E("Decryption failed: tag verification failed. error_code: %d", err);
            err = X_RET_ERROR;
        }

    }

    free(inbuf);
    free(outbuf);
    EVP_CIPHER_CTX_free(ctx);

    return err;
}
