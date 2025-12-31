#define XLOG_MOD "upgrade"
#include "main.h"
#include "exec.h"
#include "upgrade.h"
#include "os_file.h"
#include "checkout.h"
#include "xlog.h"
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define TEMPORARY_TARGZ_PATH "/tmp/upgrade_firmware.tar.gz"
#define FIRMWARE_EXTRACTED_DIR "/tmp/firmware_extracted"

#define AES_GCM_KEY_LEN  ( 16 )
#define AES_GCM_IV_LEN  ( 12 )
#define AES_GCM_TAG_LEN ( 16 )
#define RSA_SIGNATURE_LEN ( 256 )

const uint8_t magic[] = { 'I', 'O', 'T', 'A' };
const uint8_t key[AES_GCM_KEY_LEN] = {0XE9, 0X29, 0X95, 0XAA, 0X05, 0XBD, 0XF2, 0X89, 0XC4, 0X71, 0XDC, 0X7F, 0X5C, 0X13, 0X34, 0XCD};

static char *firmware_path = NULL;
static xbool_t skip_firmware_auth = xFALSE;
static xbool_t skip_firmware_verify = xFALSE;
static xbool_t upgrade_in_place = xFALSE;
static char *key_path = NULL;
static int stream_count = 4096;

#pragma pack(push, 1)
typedef struct {
    uint8_t magic[4];
    char datetime[20];
    uint32_t size;
    uint8_t iv[AES_GCM_IV_LEN];
    uint8_t reserved[12];
} image_header_t;
#pragma pack(pop)

static int upgrade_feature_entry();
static void use_upgrade_feature(xoptions context)
{ register_feature_function(upgrade_feature_entry); }

err_t upgrade_usage_init(xoptions root) {
    if (!root)
        return X_RET_INVAL;

    xoptions upgrade = xoptions_create_subcommand(root, "upgrade", "Perform a system firmware upgrade.");
    xoptions_set_posthook(upgrade, use_upgrade_feature);
    xoptions_add_string(upgrade, 'i', "image", "<firmware.iota>", "Path to the firmware image file (.iota)", &firmware_path, xTRUE);
    xoptions_add_boolean(upgrade, '\0', "skip-auth", "Bypass authentication tag validation (insecure)", &skip_firmware_auth);
    xoptions_add_boolean(upgrade, '\0', "skip-verify", "Bypass digital signature verification (insecure)", &skip_firmware_verify);
    xoptions_add_number(upgrade, 's', "stream-count", "<count>", "Number of bytes per data chunk for streaming decryption and verification", &stream_count, xFALSE);
    xoptions_add_string(upgrade, '\0', "verify", "<public_key.pem>", "Path to the public key PEM file for signature validation", &key_path, xFALSE);
    xoptions_add_boolean(upgrade, '\0', "in-place", "Update the current partition directly instead of switching", &upgrade_in_place);

    return X_RET_OK;
}

static err_t stream_decrypt_gcm(FILE *in_fp,
                                FILE *out_fp,
                                const uint8_t key[AES_GCM_KEY_LEN],
                                const uint8_t iv[AES_GCM_IV_LEN],
                                size_t data_size,
                                const uint8_t tag[AES_GCM_TAG_LEN],
                                int stream_count,
                                xbool_t skip_auth_tag);

static err_t verify_rsa_signature(FILE *in,
                                  size_t size,
                                  const uint8_t *signature,
                                  size_t signature_size,
                                  const char *public_key_pem_path);

static err_t unpack_firmware_package(const char *tar_gz_path, const char *output_dir);
static err_t install_firmware(const char *firmware_dir);
static err_t cleanup_temporary_resources();

int upgrade_feature_entry() {
    if (!firmware_path) {
        XLOG_E("No update image specified.");
        return X_RET_INVAL;
    }

    XLOG_I("Starting upgrade, firmware package: '%s', verification public key file: '%s'",
           firmware_path, key_path ? key_path : "(none)");

    XLOG_I("Stream decryption signature verification, single stream %d bytes", stream_count);

    FILE *in = os_file_open(firmware_path, "rb");
    if (!in) {
        XLOG_E("Failed to open update image: %s", firmware_path);
        return X_RET_ERROR;
    }

    err_t err = X_RET_OK;
    image_header_t header = {0};
    uint8_t tag[AES_GCM_TAG_LEN] = {0};
    uint8_t signature[RSA_SIGNATURE_LEN] = {0};

    // Read IOTA header
    size_t header_read_size = fread(&header, 1, sizeof(image_header_t), in);
    if (header_read_size != sizeof(image_header_t)) {
        XLOG_E("Failed to read image header.");
        os_file_close(in);
        return X_RET_ERROR;
    }

    XLOG_D("Firmware header:");
    XLOG_D(" Magic: %c%c%c%c", header.magic[0], header.magic[1], header.magic[2], header.magic[3]);
    XLOG_D(" Datetime: %.*s", (int)sizeof(header.datetime), header.datetime);
    XLOG_D(" Size: %u", header.size);
    XLOG_D(" IV: %x%x%x%x%x%x%x%x%x%x%x%x", 
           header.iv[0], header.iv[1], header.iv[2], header.iv[3],
           header.iv[4], header.iv[5], header.iv[6], header.iv[7],
           header.iv[8], header.iv[9], header.iv[10], header.iv[11]);

    // Read IOTA AES-GCM tag
    fseek(in, sizeof(image_header_t) + header.size - AES_GCM_TAG_LEN, SEEK_SET);
    size_t tag_read_size = fread(tag, 1, sizeof(tag), in);
    if (tag_read_size != sizeof(tag)) {
        XLOG_E("Failed to read firmware tag.");
        os_file_close(in);
        return X_RET_ERROR;
    }
    // XLOG_HEX_DUMP("Image tag:", tag, sizeof(tag));

    // Read Signature
    fseek(in, -RSA_SIGNATURE_LEN, SEEK_END);
    size_t sig_read_size = fread(signature, 1, sizeof(signature), in);
    if (sig_read_size != sizeof(signature)) {
        XLOG_E("Failed to read firmware signature.");
        os_file_close(in);
        return X_RET_ERROR;
    }
    // XLOG_HEX_DUMP("Image signature:", signature, sizeof(signature));

    XLOG_I("Checking firmware magic");
    // Validate magic
    if (memcmp(header.magic, magic, sizeof(magic)) != 0) {
        XLOG_E("Invalid firmware magic.");
        os_file_close(in);
        return X_RET_BADFMT;
    }

    // Verify signature
    if (!skip_firmware_verify) {
        if (key_path == NULL) {
            XLOG_E("No public key PEM file specified for signature verification.");
            os_file_close(in);
            return X_RET_INVAL;
        }

        XLOG_I("Verifying image signature");

        fseek(in, 0, SEEK_SET); // Seek back to the beginning for signature verification
        err = verify_rsa_signature(in,
                                   sizeof(image_header_t) + header.size,
                                   signature,
                                   sizeof(signature),
                                   key_path);

        if (err != X_RET_OK) {
            XLOG_E("Image signature verification failed");
            os_file_close(in);
            return err;
        }

        XLOG_I("Verify OK. firmware signature is valid");
    } else {
        XLOG_W("Skipping image signature verification as per user request");
    }

    XLOG_I("Decrypting firmware package");
    // Seek to the start of encrypted data
    fseek(in, sizeof(image_header_t), SEEK_SET);
    FILE *out = os_file_open(TEMPORARY_TARGZ_PATH, "wb");
    if (!out) {
        XLOG_E("Failed to open output firmware file.");
        os_file_close(in);
        return X_RET_ERROR;
    }

    err = stream_decrypt_gcm(in,
                             out,
                             key,
                             header.iv,
                             (size_t)header.size,
                             tag,
                             stream_count,
                             skip_firmware_auth);
    if (err != X_RET_OK) {
        os_file_close(in);
        os_file_close(out);
        return err;
    }

    XLOG_I("Firmware package decrypted successfully");

    os_file_close(out);
    os_file_close(in);

    if (upgrade_in_place) {
        XLOG_I("Performing In-Place update mode");
        XLOG_I("Skip mounting inactive partition");
    } else {
        XLOG_I("Performing Standard update mode");
        err = mount_inactive_partition();
        if (err != X_RET_OK) {
            XLOG_E("Failed to mount inactive partition");
            goto exit;
        }
    }

    XLOG_I("Unpacking firmware package");
    err = unpack_firmware_package(TEMPORARY_TARGZ_PATH, FIRMWARE_EXTRACTED_DIR);
    if (err != X_RET_OK) {
        XLOG_E("Failed to unpack firmware package");
        goto exit;
    }

    XLOG_I("Installing firmware");
    err = install_firmware(FIRMWARE_EXTRACTED_DIR);
    if (err != X_RET_OK) {
        XLOG_E("Failed to install firmware");
        goto exit;
    }

    XLOG_I("Firmware upgrade completed successfully");

exit:
    if (!upgrade_in_place) unmount_inactive_partition();

    cleanup_temporary_resources();

    return err;
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
    size_t total_size = data_size - AES_GCM_TAG_LEN;

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

        XLOG_T("Decrypting... %zu/%zu bytes processed (%f %%).",
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
            XLOG_I("Decrypted %ld bytes successfully", processed_size);
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


static err_t verify_rsa_signature(FILE *in,
                                  size_t size,
                                  const uint8_t *signature,
                                  size_t signature_size,
                                  const char *public_key_pem_path)
{
    EVP_PKEY *pubkey = NULL;
    EVP_MD_CTX *ctx = NULL;
    FILE *fp = NULL;
    unsigned char buf[stream_count];
    size_t n = 0;
    size_t read_bytes = 0;
    err_t err = X_RET_OK;

    FILE *public_key_file = os_file_open(public_key_pem_path, "rb");
    if (!public_key_file) goto end;

    fp = os_file_open(public_key_pem_path, "rb");
    pubkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    if (!pubkey) goto end;
    fclose(fp); fp = NULL;

    XLOG_D("Loaded public key (PEM) from %s, not displaying for security reasons.", public_key_pem_path);

    ctx = EVP_MD_CTX_new();
    if (!ctx) goto end;

    if (EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pubkey) != 1)
        goto end;

    while (read_bytes < size) {
        size_t remaining_size = size - read_bytes;
        n = fread(buf, 1, xMIN(remaining_size, sizeof(buf)), in);
        if (n == 0) break;
        if (EVP_DigestVerifyUpdate(ctx, buf, n) != 1)
            goto end;

        XLOG_T("Calculating... %zu/%zu bytes processed (%f %%).",
               read_bytes + n, size, ((double)(read_bytes + n) / size) * 100.0);

        read_bytes += n;
    }

    XLOG_D("Finalizing signature verification");
    int ret = EVP_DigestVerifyFinal(ctx, signature, signature_size);

    if (ret == 1) {
        XLOG_D("Verification successful: signature is valid.");
        err = X_RET_OK; // Signature is valid
    } else if (ret == 0) {
        XLOG_E("Signature verification failed: invalid signature.");
        err = X_RET_ERROR; // Invalid signature
    } else {
        XLOG_E("Signature verification error: %s", ERR_reason_error_string(ERR_get_error()));
        err = X_RET_ERROR; // Some other error
    }


end:
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubkey);

    return err;
}

static err_t unpack_firmware_package(const char *tar_gz_path, const char *output_dir) {
    if (!os_file_exist(tar_gz_path)) {
        XLOG_E("Firmware package file does not exist: %s", tar_gz_path);
        return X_RET_NOTENT;
    }

    xstring cmd = xstring_init_format("mkdir -p %s; tar --warning=none -xzf %s -C %s", output_dir, tar_gz_path, output_dir);
    exec_t output = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);

    if (!exec_success(output)) {
        XLOG_E("Failed to unpack firmware package");
        exec_free(output);
        return X_RET_ERROR;
    }

    return X_RET_OK;
}

static err_t install_firmware(const char *firmware_dir) {
    xstring cmd = xstring_init_format("cp -r %s/* %s", firmware_dir,
                                      upgrade_in_place ? "/" : INACTIVE_PARTITION_MOUNT_POINT);
    exec_t output = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);

    if (!exec_success(output)) {
        XLOG_E("Failed to install firmware");
        exec_free(output);
        return X_RET_ERROR;
    }

    exec_free(output);
    return X_RET_OK;
}

static err_t cleanup_temporary_resources() {
    XLOG_D("Cleaning up temporary resources");

    xstring cmd = xstring_init_format("rm -rf %s %s", FIRMWARE_EXTRACTED_DIR, TEMPORARY_TARGZ_PATH);
    exec_t output = exec_command(xstring_to_string(&cmd));
    xstring_free(&cmd);
    exec_free(output);

    return X_RET_OK;
}

