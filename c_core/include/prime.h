
/**
 * prime.h - 大素数生成库（基于bignum.h）
 * 实现Miller-Rabin素性测试和优化的素数生成
 */

#ifndef PRIME_H
#define PRIME_H

#include "bignum.h"
#include <stdbool.h>

/* ========== 素性测试配置 ========== */
#define PRIME_MILLER_RABIN_ROUNDS 40    // Miller-Rabin测试轮数（安全性 2^-80）
#define PRIME_SMALL_PRIMES_COUNT 168    // 前168个素数（<1000）用于预筛选
#define PRIME_TRIAL_DIVISION_LIMIT 2000 // 试除法的素数上限

/* ========== 素性测试函数 ========== */

/**
 * 试除法预筛选
 * 使用小素数快速排除明显的合数
 * 返回: true=可能是素数, false=确定是合数
 */
bool prime_trial_division(const BigNum *n);

/**
 * Miller-Rabin素性测试（单次）
 */
bool prime_miller_rabin_test(const BigNum *n, const BigNum *base);

/**
 * Miller-Rabin素性测试（多轮）
 */
bool prime_is_probable_prime(const BigNum *n, int rounds);

/**
 * 快速素性测试（组合优化）
 * 1. 基本检查（偶数、小于2等）
 * 2. 小素数试除
 * 3. Miller-Rabin测试
 * 返回: true=极大概率是素数, false=确定是合数
 */
bool prime_is_prime(const BigNum *n);

/* ========== 素数生成函数 ========== */

/**
 * 生成指定位数的随机素数
 */
BigNum* prime_generate(int bits);

/**
 * 生成指定位数的随机素数（优化版）
 */
int prime_generate_ex(BigNum *result, int bits);


/* ========== 辅助函数 ========== */

/**
 * 获取素数生成统计信息（可选，用于调试）
 */
typedef struct {
    int candidates_tested;      // 测试的候选数总数
    int trial_division_passed;  // 通过试除法的数量
    int miller_rabin_passed;    // 通过Miller-Rabin的数量
} PrimeGenStats;

void prime_reset_stats(void);
PrimeGenStats prime_get_stats(void);

#endif /* PRIME_H */
