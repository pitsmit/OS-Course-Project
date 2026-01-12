#include <linux/module.h> 
#include <linux/kernel.h>
#include "encrypt.h"

static const u8 xor_key[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
static const int key_len = sizeof(xor_key);

char encrypted[BUF_SIZE];

void encrypt(const u8 *in, u8 *out, size_t len) {
    for (size_t i = 0; i < len; i++) {
        out[i] = in[i] ^ xor_key[i % key_len];
    }
}
EXPORT_SYMBOL(encrypt);

void decrypt(const u8 *in, u8 *out, size_t len) {
    encrypt(in, out, len);
}
EXPORT_SYMBOL(decrypt);