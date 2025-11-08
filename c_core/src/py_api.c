/**
 * py_api.c - python图形界面接口函数实现
 */

#include "py_api.h"
#include "rsa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* ========== Python API 实现 ========== */

char *py_generate_key(int bits)
{
    RSAKey *key = rsa_generate_key(bits);
    if (!key) return NULL;

    char *public_key = rsa_export_public_key_json(key);
    char *private_key = rsa_export_private_key_json(key);

    size_t buf_size = strlen(public_key) + strlen(private_key) + 128;
    char *json = (char *)malloc(buf_size);
    if (!json) {
        free(public_key);
        free(private_key);
        rsa_free_key(key);
        return NULL;
    }

    snprintf(json, buf_size, "{\
    \"public_key\": %s \
    ,\
    \"private_key\": %s\
    }", public_key, private_key);

    free(public_key);
    free(private_key);
    rsa_free_key(key);

    return json;
}

char *py_rsa_encrypt(const char *n_hex, const char *e_hex, const char *plaintext_hex)
{
    BigNum *n = bn_new();
    BigNum *e = bn_new();
    BigNum *m = bn_new();
    BigNum *c = bn_new();

    bn_from_hex(n, n_hex);
    bn_from_hex(e, e_hex);
    bn_from_hex(m, plaintext_hex);

    RSAKey *key = rsa_key_new(n, e, NULL, NULL, NULL, NULL, NULL, NULL, 0);
    rsa_encrypt(c, m, key);

    char *out = bn_to_hex(c);

    bn_free(m); bn_free(c);
    rsa_free_key(key);

    return out;
}

char *py_rsa_decrypt(const char *n_hex, const char *d_hex,
                   const char *p_hex, const char *q_hex,
                   const char *dp_hex, const char *dq_hex,
                   const char *qinv_hex,
                   const char *ciphertext_hex)
{
    BigNum *n = bn_new();
    BigNum *d = bn_new();
    BigNum *p = bn_new();
    BigNum *q = bn_new();
    BigNum *dp = bn_new();
    BigNum *dq = bn_new();
    BigNum *qinv = bn_new();
    BigNum *c = bn_new();
    BigNum *m = bn_new();

    bn_from_hex(n, n_hex);
    bn_from_hex(d, d_hex);
    bn_from_hex(p, p_hex);
    bn_from_hex(q, q_hex);
    bn_from_hex(dp, dp_hex);
    bn_from_hex(dq, dq_hex);
    bn_from_hex(qinv, qinv_hex);
    bn_from_hex(c, ciphertext_hex);

    RSAKey *key = rsa_key_new(n, NULL, d, p, q, dp, dq, qinv, 0);
    rsa_decrypt_crt(m, c, key);

    char *out = bn_to_hex(m);

    bn_free(c); bn_free(m);
    rsa_free_key(key);

    return out;
}

char *py_rsa_sign(const char *n_hex, const char *d_hex,
               const char *p_hex, const char *q_hex,
               const char *dp_hex, const char *dq_hex,
               const char *qinv_hex,
               const char *message_hex)
{
    BigNum *n = bn_new();
    BigNum *d = bn_new();
    BigNum *p = bn_new();
    BigNum *q = bn_new();
    BigNum *dp = bn_new();
    BigNum *dq = bn_new();
    BigNum *qinv = bn_new();
    BigNum *msg = bn_new();
    BigNum *sig = bn_new();

    bn_from_hex(n, n_hex);
    bn_from_hex(d, d_hex);
    bn_from_hex(p, p_hex);
    bn_from_hex(q, q_hex);
    bn_from_hex(dp, dp_hex);
    bn_from_hex(dq, dq_hex);
    bn_from_hex(qinv, qinv_hex);
    bn_from_hex(msg, message_hex);

    RSAKey *key = rsa_key_new(n, NULL, d, p, q, dp, dq, qinv, 0);
    rsa_sign(sig, msg, key);

    char *out = bn_to_hex(sig);

    bn_free(msg); bn_free(sig);
    rsa_free_key(key);

    return out;
}


bool py_rsa_verify(const char *n_hex, const char *e_hex,
               const char *message_hex, const char *signature_hex)
{
    BigNum *n = bn_new();
    BigNum *e = bn_new();
    BigNum *msg = bn_new();
    BigNum *sig = bn_new();

    bn_from_hex(n, n_hex);
    bn_from_hex(e, e_hex);
    bn_from_hex(msg, message_hex);
    bn_from_hex(sig, signature_hex);

    RSAKey *key = rsa_key_new(n, e, NULL, NULL, NULL, NULL, NULL, NULL, 0);
    bool result = rsa_verify(msg, sig, key) ? true : false;

    bn_free(msg); bn_free(sig);
    rsa_free_key(key);

    return result;
}