/**
 * rsa.h - RSA加密核心功能
 */

#ifndef RSA_H
#define RSA_H

#include "bignum.h"
#include "sha256.h"
#include <stdint.h>
#include <stddef.h>

/* ========== RSA密钥结构 ========== */

typedef struct {
    BigNum *n;       // 模数 n = p * q
    BigNum *e;       // 公钥指数
    BigNum *d;       // 私钥指数
    BigNum *p;       // 素数p
    BigNum *q;       // 素数q
    
    // CRT优化参数
    BigNum *dp;      // d mod (p-1)
    BigNum *dq;      // d mod (q-1)
    BigNum *qinv;    // q^-1 mod p
    
    int bits;        // 密钥位数
} RSAKey;

/* ========== 密钥管理 ========== */

/**
 * 生成RSA密钥对
 * @param bits 密钥位数
 * @return RSA密钥对
 */
RSAKey* rsa_generate_key(int bits);

/**
 * 释放RSA密钥
 */
void rsa_free_key(RSAKey *key);

/**
 * 导出公钥（n, e）
 */
char *rsa_export_public_key_json(const RSAKey *key);

/**
 * 导出私钥
 */
char *rsa_export_private_key_json(const RSAKey *key);

/* ========== 加密/解密 ========== */

/**
 * RSA加密：c = m^e mod n
 * @param ciphertext 密文输出
 * @param message 明文
 * @param key RSA密钥
 */
void rsa_encrypt(BigNum *ciphertext, const BigNum *message, const RSAKey *key);

/**
 * RSA解密：m = c^d mod n
 * @param message 明文输出
 * @param ciphertext 密文
 * @param key RSA密钥
 */
void rsa_decrypt(BigNum *message, const BigNum *ciphertext, const RSAKey *key);

/**
 * RSA解密（CRT优化版本）
 */
void rsa_decrypt_crt(BigNum *message, const BigNum *ciphertext, const RSAKey *key);

/* ========== 签名/验证 ========== */

/**
 * RSA签名：s = H(m)^d mod n
 * @param signature 签名输出
 * @param message 消息
 * @param key RSA密钥
 */
void rsa_sign(BigNum *signature, const BigNum *message, const RSAKey *key);

/**
 * RSA验证：检查 H(m) == s^e mod n
 * @param message 消息
 * @param signature 签名
 * @param key RSA密钥
 * @return true表示验证通过
 */
bool rsa_verify(const BigNum *message, const BigNum *signature, const RSAKey *key);


RSAKey *rsa_key_new(BigNum *n, BigNum *e, BigNum *d,
                    BigNum *p, BigNum *q,
                    BigNum *dp, BigNum *dq, BigNum *qinv,
                    int bits);

#endif /* RSA_H */