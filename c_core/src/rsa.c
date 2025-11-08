/**
 * rsa.c - RSA加密核心实现
 * 包含：密钥生成、加解密、签名验证、CRT优化
 */

#include "rsa.h"
#include "prime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== 密钥生成 ========== */

RSAKey *rsa_generate_key(int bits)
{
    if (bits < 256)
    {
        fprintf(stderr, "Error: Key size must be >= 256 bits\n");
        return NULL;
    }

    RSAKey *key = malloc(sizeof(RSAKey));
    if (!key)
        return NULL;

    // 初始化
    key->n = bn_new();
    key->e = bn_new();
    key->d = bn_new();
    key->p = NULL;
    key->q = NULL;
    key->dp = bn_new();
    key->dq = bn_new();
    key->qinv = bn_new();
    key->bits = bits;

    // 设置公钥指数 e = 65537
    bn_from_int(key->e, 65537);

    // printf("Generating %d-bit RSA key...\n", bits);

    // 生成素数p和q
    int p_bits = bits / 2;
    int q_bits = bits - p_bits;

    // printf("Generating prime p (%d bits)...\n", p_bits);
    key->p = prime_generate(p_bits);

    // printf("Generating prime q (%d bits)...\n", q_bits);
    key->q = prime_generate(q_bits);

    // 确保 p != q
    while (bn_cmp(key->p, key->q) == 0)
    {
        printf("p == q, regenerating q...\r\n");
        bn_free(key->q);
        key->q = prime_generate(q_bits);
    }

    // 确保 p > q（可选，便于调试）
    if (bn_cmp(key->p, key->q) < 0)
    {
        BigNum *temp = key->p;
        key->p = key->q;
        key->q = temp;
    }

    // 计算 n = p * q
    // printf("Computing n = p * q...\n");
    bn_mul(key->n, key->p, key->q);

    // 验证n的位数
    int n_bits = bn_bit_length(key->n);
    // printf("Generated n with %d bits (expected %d)\n", n_bits, bits);

    if (n_bits < bits - 1)
    {
        fprintf(stderr, "Warning: n is only %d bits (expected ~%d)\n", n_bits, bits);
    }

    // 计算 φ(n) = (p-1)(q-1)
    // printf("Computing φ(n)...\n");
    BigNum *p_minus_1 = bn_copy(key->p);
    BigNum *q_minus_1 = bn_copy(key->q);
    BigNum *one = bn_new();
    bn_from_int(one, 1);

    bn_sub(p_minus_1, p_minus_1, one);
    bn_sub(q_minus_1, q_minus_1, one);

    BigNum *phi = bn_new();
    bn_mul(phi, p_minus_1, q_minus_1);

    // 计算私钥 d = e^-1 mod φ(n)
    // printf("Computing private key d = e^-1 mod φ(n)...\n");
    bn_mod_inverse(key->d, key->e, phi);

    if (bn_is_zero(key->d))
    {
        fprintf(stderr, "Error: Failed to compute modular inverse\n");
        bn_free(one);
        bn_free(p_minus_1);
        bn_free(q_minus_1);
        bn_free(phi);
        rsa_free_key(key);
        return NULL;
    }

    // 计算CRT参数（加速解密）
    // printf("Computing CRT parameters...\n");
    bn_mod(key->dp, key->d, p_minus_1);        // dp = d mod (p-1)
    bn_mod(key->dq, key->d, q_minus_1);        // dq = d mod (q-1)
    bn_mod_inverse(key->qinv, key->q, key->p); // qinv = q^-1 mod p

    // printf("Key generation complete!\r\n");
    // printf("  n: %d bits\r\n", bn_bit_length(key->n));
    // printf("  e: %llu\r\n", (unsigned long long)bn_to_uint64(key->e));

    // 清理临时变量
    bn_free(one);
    bn_free(p_minus_1);
    bn_free(q_minus_1);
    bn_free(phi);

    return key;
}

void rsa_free_key(RSAKey *key)
{
    if (!key)
        return;

    bn_free(key->n);
    bn_free(key->e);
    bn_free(key->d);
    bn_free(key->p);
    bn_free(key->q);
    bn_free(key->dp);
    bn_free(key->dq);
    bn_free(key->qinv);
    free(key);
}

char *rsa_export_public_key_json(const RSAKey *key)
{
    // 计算大概需要的缓冲区长度
    size_t buf_size = 1024 + key->n->length * 8 + key->e->length * 8;
    char *buf = malloc(buf_size);
    if (!buf)
        return NULL;

    char *n_hex = bn_to_hex(key->n);
    char *e_hex = bn_to_hex(key->e);

    (void)snprintf(buf, buf_size,
                       "{\n"
                       "  \"n\": \"%s\",\n"
                       "  \"e\": \"%s\"\n"
                       "}",
                       n_hex, e_hex);

    free(n_hex);
    free(e_hex);

    return buf;
}

char *rsa_export_private_key_json(const RSAKey *key)
{
    size_t buf_size = 4096 + key->n->length * 8 + key->d->length * 8 +
                      key->p->length * 8 + key->q->length * 8 +
                      key->dp->length * 8 + key->dq->length * 8 + key->qinv->length * 8;

    char *buf = malloc(buf_size);
    if (!buf)
        return NULL;

    char *n_hex = bn_to_hex(key->n);
    char *d_hex = bn_to_hex(key->d);
    char *p_hex = bn_to_hex(key->p);
    char *q_hex = bn_to_hex(key->q);
    char *dp_hex = bn_to_hex(key->dp);
    char *dq_hex = bn_to_hex(key->dq);
    char *qinv_hex = bn_to_hex(key->qinv);

    (void)snprintf(buf, buf_size,
                       "{\n"
                       "  \"n\": \"%s\",\n"
                       "  \"d\": \"%s\",\n"
                       "  \"p\": \"%s\",\n"
                       "  \"q\": \"%s\",\n"
                       "  \"dp\": \"%s\",\n"
                       "  \"dq\": \"%s\",\n"
                       "  \"qinv\": \"%s\"\n"
                       "}",
                       n_hex, d_hex, p_hex, q_hex, dp_hex, dq_hex, qinv_hex);

    free(n_hex);
    free(d_hex);
    free(p_hex);
    free(q_hex);
    free(dp_hex);
    free(dq_hex);
    free(qinv_hex);

    return buf;
}

/* ========== 加密/解密 ========== */

void rsa_encrypt(BigNum *ciphertext, const BigNum *message, const RSAKey *key)
{
    // c = m^e mod n

    // 检查消息是否小于n
    if (bn_cmp(message, key->n) >= 0)
    {
        fprintf(stderr, "Error: Message must be smaller than n\n");
        bn_clear(ciphertext);
        return;
    }
    if (!key->n || !key->e)
    {
        fprintf(stderr, "Public Key Error\n");
        bn_clear(ciphertext);
        return;
    }

    bn_mod_exp(ciphertext, message, key->e, key->n);
}

void rsa_decrypt(BigNum *message, const BigNum *ciphertext, const RSAKey *key)
{
    if (!(key->d && key->dp && key->dq && key->p && key->q && key->qinv))
    {
        fprintf(stderr, "Private Key Error\n");
        return;
    }
    // m = c^d mod n
    bn_mod_exp(message, ciphertext, key->d, key->n);
}

void rsa_decrypt_crt(BigNum *message, const BigNum *ciphertext, const RSAKey *key)
{
    if (!(key->d && key->dp && key->dq && key->p && key->q && key->qinv))
    {
        fprintf(stderr, "Private Key Error\n");
        return;
    }
    BigNum *m1 = bn_new();
    BigNum *m2 = bn_new();
    BigNum *h = bn_new();
    BigNum *tmp = bn_new();

    // 计算 m1 = c^dp mod p, m2 = c^dq mod q
    bn_mod_exp(m1, ciphertext, key->dp, key->p);
    bn_mod_exp(m2, ciphertext, key->dq, key->q);

    // h = (m1 - m2)
    bn_sub(h, m1, m2);

    // 如果 h < 0，加上 p
    if (h->sign < 0)
        bn_add(h, h, key->p);

    // h = h * qinv mod p
    bn_mod_mul(h, h, key->qinv, key->p);

    // message = m2 + h * q
    bn_mul(tmp, h, key->q);
    bn_add(message, m2, tmp);

    // 最终模 n
    bn_mod(message, message, key->n);

    // 释放临时变量
    bn_free(m1);
    bn_free(m2);
    bn_free(h);
    bn_free(tmp);
}

/* ========== 签名/验证 ========== */

void rsa_sign(BigNum *signature, const BigNum *message, const RSAKey *key)
{
    // s = H(m)^d mod n
    // 签名就是用私钥"解密"哈希值
    char *message_hex = bn_to_hex(message); // "0x..." ASCII
    if(message_hex)
        message_hex += 2;

    char hash_bin[65];
    sha256_easy_hash_hex(message_hex, strlen(message_hex), hash_bin); // 假设 sha256_easy_hash_hex 可以接受 ASCII hex

    free(message_hex - 2);

    // 转为 BigNum
    BigNum* message_hash = bn_new();
    bn_from_bytes(message_hash, (const uint8_t*)hash_bin, 32);

    // 签名 = H(m)^d mod n
    rsa_decrypt_crt(signature, message_hash, key);

    bn_free(message_hash);
}

bool rsa_verify(const BigNum *message, const BigNum *signature, const RSAKey *key)
{
    // 1. 先将 BigNum 转为 hex 字符串
    char *message_hex = bn_to_hex(message); // "0x..." ASCII
    if (message_hex)
        message_hex += 2; // 跳过 "0x"

    // 2. 计算 SHA-256
    char hash_bin[65];
    sha256_easy_hash_hex(message_hex, strlen(message_hex), hash_bin);

    free(message_hex - 2); // 注意释放原始指针

    // 3. 转为 BigNum
    BigNum *message_hash = bn_new();
    bn_from_bytes(message_hash, (const uint8_t*)hash_bin, 32);

    // 4. 用公钥验证：s^e mod n
    BigNum *decrypted = bn_new();
    bn_mod_exp(decrypted, signature, key->e, key->n);

    bool valid = (bn_cmp(decrypted, message_hash) == 0);

    bn_free(message_hash);
    bn_free(decrypted);
    return valid;
}

/* ========== 字节串处理 ========== */

BigNum *bytes_to_bignum(const uint8_t *bytes, size_t len)
{
    if (len == 0)
    {
        return bn_new();
    }

    int words = (int)((len + 3) / 4);
    BigNum *bn = bn_new_size(words);

    memset(bn->data, 0, words * sizeof(uint32_t));

    // 大端序字节转小端序32位字
    for (size_t i = 0; i < len; i++)
    {
        int word_idx = (int)(i / 4);
        int byte_idx = (int)(i % 4);
        bn->data[word_idx] |= ((uint32_t)bytes[len - 1 - i]) << (byte_idx * 8);
    }

    bn->length = words;
    bn->sign = 1;
    bn_cleanup(bn);

    return bn;
}

size_t bignum_to_bytes(uint8_t *bytes, size_t max_len, const BigNum *bn)
{
    if (bn_is_zero(bn))
    {
        if (max_len > 0)
            bytes[0] = 0;
        return 1;
    }

    int bits = bn_bit_length(bn);
    size_t len = (size_t)((bits + 7) / 8);

    if (len > max_len)
    {
        fprintf(stderr, "Error: Buffer too small for bignum (%zu bytes needed, %zu available)\n",
                len, max_len);
        return 0;
    }

    // 小端序32位字转大端序字节
    memset(bytes, 0, len);

    for (size_t i = 0; i < len; i++)
    {
        int word_idx = (int)(i / 4);
        int byte_idx = (int)(i % 4);

        if (word_idx < bn->length)
        {
            bytes[len - 1 - i] = (uint8_t)((bn->data[word_idx] >> (byte_idx * 8)) & 0xFF);
        }
    }

    return len;
}

RSAKey* rsa_key_new(BigNum *n, BigNum *e, BigNum *d,
                    BigNum *p, BigNum *q,
                    BigNum *dp, BigNum *dq, BigNum *qinv,
                    int bits)
{
    RSAKey *key = (RSAKey*)malloc(sizeof(RSAKey));
    if (!key) return NULL;

    // 直接赋值指针（不拷贝，如果需要拷贝 BigNum，另写 bn_copy）
    key->n = n;
    key->e = e;
    key->d = d;
    key->p = p;
    key->q = q;
    key->dp = dp;
    key->dq = dq;
    key->qinv = qinv;
    key->bits = bits;

    return key;
}