#ifndef ENCRYPT_H
#define ENCRYPT_H

#define BUF_SIZE 10000
extern char encrypted[BUF_SIZE];

extern void encrypt(const u8 *in, u8 *out, size_t len);
extern void decrypt(const u8 *in, u8 *out, size_t len);

#endif