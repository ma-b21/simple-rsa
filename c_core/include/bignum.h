/**
 * bignum.h - RSA专用大数运算库
 * 精简实现，只包含RSA必需的功能
 */

#ifndef BIGNUM_H
#define BIGNUM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

/* 大数结构 - 使用32位字存储，小端序 */
typedef struct {
    uint32_t *data;      // 数据数组（小端序：data[0]是最低位）
    int length;          // 有效字数量
    int capacity;        // 分配的容量
    int sign;            // 符号: 1=正数, -1=负数, 0=零
} BigNum;

/* ========== 内存管理 ========== */
BigNum* bn_new(void);
BigNum* bn_new_size(int capacity);
void bn_free(BigNum *bn);
BigNum* bn_copy(const BigNum *bn);
void bn_resize(BigNum *bn, int new_capacity);
void bn_clear(BigNum *bn);

/* ========== 初始化和转换 ========== */
void bn_from_int(BigNum *bn, uint64_t val);
void bn_from_hex(BigNum *bn, const char *hex);
char* bn_to_hex(const BigNum *bn);
uint64_t bn_to_uint64(const BigNum *bn);
void bn_from_bytes(BigNum *bn, const uint8_t *bytes, size_t len);

/* ========== 比较操作 ========== */
int bn_cmp(const BigNum *a, const BigNum *b);           // 返回: -1, 0, 1
int bn_cmp_abs(const BigNum *a, const BigNum *b);       // 比较绝对值
bool bn_is_zero(const BigNum *bn);
bool bn_is_odd(const BigNum *bn);
bool bn_is_even(const BigNum *bn);

/* ========== 位操作 ========== */
int bn_bit_length(const BigNum *bn);                    // 返回二进制位数
void bn_set_bit(BigNum *bn, int pos);
int bn_get_bit(const BigNum *bn, int pos);
void bn_lshift(BigNum *result, const BigNum *bn, int bits);  // result = bn << bits
void bn_rshift(BigNum *result, const BigNum *bn, int bits);  // result = bn >> bits

/* ========== 基础运算 ========== */
void bn_add(BigNum *result, const BigNum *a, const BigNum *b);
void bn_sub(BigNum *result, const BigNum *a, const BigNum *b);
void bn_mul_naive(BigNum *result, const BigNum *a, const BigNum *b);
void bn_mul(BigNum *result, const BigNum *a, const BigNum *b);
void bn_div(BigNum *quotient, BigNum *remainder, const BigNum *a, const BigNum *b);
void bn_mod(BigNum *result, const BigNum *a, const BigNum *b);

/* ========== 模运算（RSA核心）========== */
void bn_mod_add(BigNum *result, const BigNum *a, const BigNum *b, const BigNum *mod);
void bn_mod_sub(BigNum *result, const BigNum *a, const BigNum *b, const BigNum *mod);
void bn_mod_mul(BigNum *result, const BigNum *a, const BigNum *b, const BigNum *mod);
void bn_mod_exp(BigNum *result, const BigNum *base, const BigNum *exp, const BigNum *mod);
void bn_mod_inverse(BigNum *result, const BigNum *a, const BigNum *mod);

/* ========== 随机数生成 ========== */
void bn_rand(BigNum *bn, int bits);                     // 生成bits位随机数
void bn_rand_range(BigNum *bn, const BigNum *max);      // 生成[0, max)的随机数

/* ========== 辅助函数 ========== */
void bn_print(const char *label, const BigNum *bn);
void bn_cleanup(BigNum *bn);  // 移除前导零

#define BN_MUL_KARATSUBA_THRESHOLD 128  // 超过128个32位字时启用Karatsuba (4096bit以上)  

#endif /* BIGNUM_H */