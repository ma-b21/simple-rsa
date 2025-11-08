/**
 * py_api.h - python图形界面接口函数
 */
#ifndef PY_API_H
#define PY_API_H

#include "rsa.h"

/**
 * 生成RSA密钥
 * @param bits: 密钥位数
 * @return JSON字符串，包含公钥和私钥，内部BigNum以hex字符串表示
 */
char *py_generate_key(int bits);

/**
 * 公钥加密
 * @param n_hex: 模数 n 的 hex 字符串
 * @param e_hex: 公钥指数 e 的 hex 字符串
 * @param plaintext_hex: 待加密明文 hex 字符串
 * @return ciphertext_hex: 加密后的 hex 字符串
 */
char *py_rsa_encrypt(const char *n_hex, const char *e_hex, const char *plaintext_hex);

/**
 * 私钥解密
 * @param n_hex: 模数 n 的 hex 字符串
 * @param d_hex: 私钥指数 d 的 hex 字符串
 * @param ciphertext_hex: 待解密密文 hex 字符串
 * @return plaintext_hex: 解密后的 hex 字符串
 */
char *py_rsa_decrypt(const char *n_hex, const char *d_hex,
                     const char *p_hex, const char *q_hex,
                     const char *dp_hex, const char *dq_hex,
                     const char *qinv_hex,
                     const char *ciphertext_hex);

/**
 * 私钥签名
 * @param n_hex: 模数 n 的 hex 字符串
 * @param d_hex: 私钥指数 d 的 hex 字符串
 * @param message_hex: 待签名消息 hex 字符串
 * @return signature_hex: 签名 hex 字符串
 */
char *py_rsa_sign(const char *n_hex, const char *d_hex,
                  const char *p_hex, const char *q_hex,
                  const char *dp_hex, const char *dq_hex,
                  const char *qinv_hex,
                  const char *message_hex);

/**
 * 公钥验签
 * @param n_hex: 模数 n 的 hex 字符串
 * @param e_hex: 公钥指数 e 的 hex 字符串
 * @param message_hex: 原消息 hex 字符串
 * @param signature_hex: 签名 hex 字符串
 * @return 验签结果，1 表示成功，0 表示失败
 */
bool py_rsa_verify(const char *n_hex, const char *e_hex,
                   const char *message_hex, const char *signature_hex);

#endif /*PY_API_H*/