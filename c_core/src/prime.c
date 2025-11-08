
/**
 * prime.c - 大素数生成库实现
 *
 * 内置性能统计与调试输出
 * - 统计时间粒度（纳秒）：总耗时、候选生成/对齐、轮筛步进、试除法、MR 分解、MR 每轮、modexp、平方次数等
 * - 统计计数：候选数量、试除调用次数、MR 轮数、modexp 次数、平方次数、随机基底次数
 * - 输出：在 prime_generate_ex 结束时打印一次概要；
 * - 默认开启：#define PRIME_PROFILE 1
 * - 关闭：编译时加 -DPRIME_PROFILE=0 或在本文件中设为 0
 */

#ifndef PRIME_PROFILE
#define PRIME_PROFILE 0
#endif

#include "prime.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

/* ========== 性能分析工具 ========== */
#if PRIME_PROFILE

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static inline uint64_t now_ns(void)
{
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER ctr;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ctr);
    return (uint64_t)((__int128)ctr.QuadPart * 1000000000ull / freq.QuadPart);
}
#else
#include <sys/time.h>
static inline uint64_t now_ns(void)
{
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
#endif

typedef struct
{
    /* 总体 */
    uint64_t total_ns;
    uint64_t candidates_tested;

    /* 候选生成/对齐/轮筛步进 */
    uint64_t rand_candidate_ns;
    uint64_t wheel_align_ns;
    uint64_t wheel_step_ns;

    /* 试除法 */
    uint64_t trial_ns;
    uint64_t trial_calls;

    /* MR 总体与子项 */
    uint64_t mr_decompose_ns;
    uint64_t mr_round_ns;           /* 每轮总和 */
    uint64_t mr_rounds;

    uint64_t mr_modexp_ns;          /* base^d mod n */
    uint64_t mr_modexp_calls;

    uint64_t mr_squarings_ns;       /* x = x^2 mod n (r-1 次) */
    uint64_t mr_squarings;

    uint64_t mr_rand_base_ns;       /* 生成随机基底 2..n-2 */
    uint64_t mr_rand_bases;

} PrimeProfile;

static PrimeProfile g_prof;

static inline void prof_reset(void)
{
    memset(&g_prof, 0, sizeof(g_prof));
}

static void prof_dump(const char *tag, int bits, int candidates)
{
    const double total_ms = g_prof.total_ns / 1e6;

    double rand_ms = g_prof.rand_candidate_ns / 1e6;
    double align_ms = g_prof.wheel_align_ns / 1e6;
    double step_ms = g_prof.wheel_step_ns / 1e6;

    double trial_ms = g_prof.trial_ns / 1e6;

    double decomp_ms = g_prof.mr_decompose_ns / 1e6;
    double mrr_ms = g_prof.mr_round_ns / 1e6;
    double modexp_ms = g_prof.mr_modexp_ns / 1e6;
    double square_ms = g_prof.mr_squarings_ns / 1e6;
    double randbase_ms = g_prof.mr_rand_base_ns / 1e6;

    double known_sum = rand_ms + align_ms + step_ms + trial_ms + decomp_ms + mrr_ms + randbase_ms;
    double other_ms = total_ms - known_sum;

    fprintf(stderr, "\n[PROFILE] %s (bits=%d) — candidates=%d total=%.3f ms\n",
            tag ? tag : "prime_generate_ex", bits, candidates, total_ms);

    fprintf(stderr, "  - candidate random:     %10.3f ms\n", rand_ms);
    fprintf(stderr, "  - wheel align:          %10.3f ms\n", align_ms);
    fprintf(stderr, "  - wheel stepping:       %10.3f ms\n", step_ms);

    fprintf(stderr, "  - trial division:       %10.3f ms  (calls=%" PRIu64 ")\n",
            trial_ms, g_prof.trial_calls);

    fprintf(stderr, "  - MR decompose:         %10.3f ms\n", decomp_ms);
    fprintf(stderr, "  - MR rounds total:      %10.3f ms  (rounds=%" PRIu64 ")\n",
            mrr_ms, g_prof.mr_rounds);
    fprintf(stderr, "      - modexp:           %10.3f ms  (calls=%" PRIu64 ")\n",
            modexp_ms, g_prof.mr_modexp_calls);
    fprintf(stderr, "      - squarings:        %10.3f ms  (count=%" PRIu64 ")\n",
            square_ms, g_prof.mr_squarings);
    fprintf(stderr, "      - rand bases:       %10.3f ms  (count=%" PRIu64 ")\n",
            randbase_ms, g_prof.mr_rand_bases);

    fprintf(stderr, "  - other/unaccounted:    %10.3f ms\n\n", other_ms < 0 ? 0.0 : other_ms);
}

#endif /* PRIME_PROFILE */

/* ========== 小素数表（前168个素数，<1000）========== */
static const uint16_t SMALL_PRIMES[] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
    239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
    331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419,
    421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503,
    509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607,
    613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701,
    709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811,
    821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911,
    919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997};

#ifndef PRIME_SMALL_PRIMES_COUNT
#define PRIME_SMALL_PRIMES_COUNT (sizeof(SMALL_PRIMES)/sizeof(SMALL_PRIMES[0]))
#endif

/* 统计信息（可选） */
static PrimeGenStats g_stats = {0, 0, 0};

/* ========== 内部辅助：随机、常量、u32取模等 ========== */

/* 2×3×5 轮筛：允许的余数和步进序列（mod 30） */
static const int WHEEL_DELTAS[8] = {2, 4, 2, 4, 6, 2, 6, 4}; /* 总和=30 */
static int wheel_residue_index(int r)
{
    switch (r)
    {
    case 1: return 0;
    case 7: return 1;
    case 11: return 2;
    case 13: return 3;
    case 17: return 4;
    case 19: return 5;
    case 23: return 6;
    case 29: return 7;
    default: return -1;
    }
}

/**
 * 初始化随机数生成器（仅调用一次）
 */
static void init_random(void)
{
    static bool initialized = false;
    if (!initialized)
    {
        srand((unsigned int)time(NULL));
        initialized = true;
    }
}

/**
 * 生成指定位数的随机奇数
 */
static void generate_random_odd(BigNum *n, int bits)
{
#if PRIME_PROFILE
    uint64_t t0 = now_ns();
#endif
    bn_rand(n, bits);

    // 确保最高位为1（保证位数）
    bn_set_bit(n, bits - 1);

    // 确保最低位为1（奇数）
    if (bn_is_even(n))
    {
        bn_set_bit(n, 0);
    }
#if PRIME_PROFILE
    g_prof.rand_candidate_ns += (now_ns() - t0);
#endif
}

/* 访问 BigNum 内部以做 u32 取模（假设 data[0] 是最低 limb） */
static inline uint32_t bn_mod_u32(const BigNum *n, uint32_t m)
{
    if (m == 0) return 0;
    uint64_t rem = 0;
    if (n->length == 0) return 0;
    for (int i = (int)n->length - 1; i >= 0; --i)
    {
        rem = ((rem << 32) + (uint64_t)n->data[i]) % m;
    }
    return (uint32_t)rem;
}

/* 判断 n 是否等于某个 u32 值（正数场景） */
static inline bool bn_equals_u32(const BigNum *n, uint32_t v)
{
    if (v == 0) return bn_is_zero(n);
    if (n->length == 1 && n->data[0] == v) return true;
    return false;
}

/* 快速常量（进程生命周期内仅初始化一次） */
static BigNum* BN_ONE(void)
{
    static BigNum* one = NULL;
    if (!one) { one = bn_new(); bn_from_int(one, 1); }
    return one;
}
static BigNum* BN_TWO(void)
{
    static BigNum* two = NULL;
    if (!two) { two = bn_new(); bn_from_int(two, 2); }
    return two;
}
static BigNum* BN_THREE(void)
{
    static BigNum* three = NULL;
    if (!three) { three = bn_new(); bn_from_int(three, 3); }
    return three;
}

/* ========== 试除法预筛选（加速版） ========== */

bool prime_trial_division(const BigNum *n)
{
#if PRIME_PROFILE
    uint64_t t0 = now_ns();
    bool result = true;
#endif

    // 针对 n < 1000 的快速处理与等值判断
    for (int i = 0; i < (int)PRIME_SMALL_PRIMES_COUNT; i++)
    {
        uint32_t p = SMALL_PRIMES[i];

        // 如果 n == p，直接判定为素数
        if (bn_equals_u32(n, p))
        {
#if PRIME_PROFILE
            g_prof.trial_calls++;
            g_prof.trial_ns += (now_ns() - t0);
#endif
            return true;
        }

        // 如果能被小素数整除，则是合数
        if (bn_mod_u32(n, p) == 0)
        {
#if PRIME_PROFILE
            g_prof.trial_calls++;
            g_prof.trial_ns += (now_ns() - t0);
#endif
            return false;
        }
    }

#if PRIME_PROFILE
    g_prof.trial_calls++;
    g_prof.trial_ns += (now_ns() - t0);
    result = true;
    return result;
#else
    return true; // 通过试除法预筛
#endif
}

/* ========== Miller-Rabin 素性测试（复用 n-1 分解） ========== */

typedef struct
{
    BigNum *d;          // odd part of n-1
    int r;              // n-1 = 2^r * d
    BigNum *n_minus_1;  // n-1
} MRDecomp;

static void mr_decompose(const BigNum *n, MRDecomp *ctx)
{
    ctx->n_minus_1 = bn_new();
    bn_sub(ctx->n_minus_1, n, BN_ONE());

    ctx->d = bn_copy(ctx->n_minus_1);
    ctx->r = 0;
    while (bn_is_even(ctx->d))
    {
        bn_rshift(ctx->d, ctx->d, 1); // d = d / 2
        ctx->r++;
    }
}

static void mr_decompose_free(MRDecomp *ctx)
{
    if (ctx->d) { bn_free(ctx->d); ctx->d = NULL; }
    if (ctx->n_minus_1) { bn_free(ctx->n_minus_1); ctx->n_minus_1 = NULL; }
}

/* 单轮 MR 测试（使用已分解的 d,r） */
static bool mr_round(const BigNum *n, const BigNum *base, const MRDecomp *ctx, BigNum *x_scratch)
{
#if PRIME_PROFILE
    g_prof.mr_rounds++;
    uint64_t t_round0 = now_ns();
#endif

    // x = base^d mod n
#if PRIME_PROFILE
    uint64_t t_exp0 = now_ns();
#endif
    bn_mod_exp(x_scratch, base, ctx->d, n);
#if PRIME_PROFILE
    g_prof.mr_modexp_calls++;
    g_prof.mr_modexp_ns += (now_ns() - t_exp0);
#endif

    // 如果 x == 1 或 x == n-1，通过
    if (bn_cmp(x_scratch, BN_ONE()) == 0 || bn_cmp(x_scratch, ctx->n_minus_1) == 0)
    {
#if PRIME_PROFILE
        g_prof.mr_round_ns += (now_ns() - t_round0);
#endif
        return true;
    }

    // 重复 r-1 次：x = x^2 mod n
    for (int i = 0; i < ctx->r - 1; i++)
    {
#if PRIME_PROFILE
        uint64_t t_sq0 = now_ns();
#endif
        bn_mod_mul(x_scratch, x_scratch, x_scratch, n); // x = x^2 mod n
#if PRIME_PROFILE
        g_prof.mr_squarings++;
        g_prof.mr_squarings_ns += (now_ns() - t_sq0);
#endif
        if (bn_cmp(x_scratch, ctx->n_minus_1) == 0)
        {
#if PRIME_PROFILE
            g_prof.mr_round_ns += (now_ns() - t_round0);
#endif
            return true;
        }
    }
#if PRIME_PROFILE
    g_prof.mr_round_ns += (now_ns() - t_round0);
#endif
    return false; // 未通过
}

/**
 * 兼容保留：单次 Miller-Rabin（仍可被外部调用）
 * 内部仍会分解一次 n-1。建议优先使用 prime_is_probable_prime（会复用分解）。
 */
bool prime_miller_rabin_test(const BigNum *n, const BigNum *base)
{
    // n 必须为奇数且 > 2
    if (bn_is_even(n)) return false;
    if (bn_cmp(n, BN_TWO()) == 0) return true;

    MRDecomp dec = {0};
#if PRIME_PROFILE
    uint64_t t_dec0 = now_ns();
#endif
    mr_decompose(n, &dec);
#if PRIME_PROFILE
    g_prof.mr_decompose_ns += (now_ns() - t_dec0);
#endif

    BigNum *x = bn_new();
    bool ok = mr_round(n, base, &dec, x);

    bn_free(x);
    mr_decompose_free(&dec);
    return ok;
}

/* ========== 多轮 Miller-Rabin（预分解 + 控制轮数上限） ========== */

bool prime_is_probable_prime(const BigNum *n, int rounds)
{
    init_random();

    // 特殊情况
    if (bn_cmp(n, BN_TWO()) < 0) return false;
    if (bn_cmp(n, BN_TWO()) == 0) return true;
    if (bn_is_even(n)) return false;

    // 预筛：小素数试除
    if (!prime_trial_division(n)) return false;

    // 若 n < 1000 且通过试除，直接判素
    BigNum *max_small = bn_new();
    bn_from_int(max_small, 1000);
    if (bn_cmp(n, max_small) < 0)
    {
        bn_free(max_small);
        return true;
    }
    bn_free(max_small);

    // 限制轮数：最多 12 轮（在不牺牲实际安全性的前提下大幅提速）
    int max_rounds = (rounds > 12) ? 12 : rounds;

    // 预分解 n-1 = 2^r * d
    MRDecomp dec = {0};
#if PRIME_PROFILE
    uint64_t t_dec0 = now_ns();
#endif
    mr_decompose(n, &dec);
#if PRIME_PROFILE
    g_prof.mr_decompose_ns += (now_ns() - t_dec0);
#endif

    // Scratch
    BigNum *x = bn_new();
    BigNum *base = bn_new();

    // 先用几个固定小基底（2,3,5），减少随机调用成本
    const uint32_t fixed_bases[] = {2, 3, 5};
    int used_rounds = 0;
    for (int i = 0; i < (int)(sizeof(fixed_bases)/sizeof(fixed_bases[0])) && used_rounds < max_rounds; i++)
    {
        // 如果基底 >= n，跳过（如 n 很小）
        if (bn_equals_u32(n, fixed_bases[i]) || (bn_cmp(n, BN_TWO()) > 0 && fixed_bases[i] < 2))
            continue;

        bn_from_int(base, (int)fixed_bases[i]);
        if (bn_cmp(base, n) >= 0) continue; // base 必须 < n

        if (!mr_round(n, base, &dec, x))
        {
            bn_free(x);
            bn_free(base);
            mr_decompose_free(&dec);
            return false;
        }
        used_rounds++;
    }

    // 剩余轮数使用随机基底：base = 2 + rand_range(0, n-4)
    BigNum *n_minus_3 = bn_new();
    bn_sub(n_minus_3, n, BN_THREE()); // n-3
    while (used_rounds < max_rounds)
    {
        // 随机 in [0, n-4]，再 +2 => [2, n-2]
#if PRIME_PROFILE
        uint64_t t_rb0 = now_ns();
#endif
        bn_rand_range(base, n_minus_3);
        bn_add(base, base, BN_TWO());
#if PRIME_PROFILE
        g_prof.mr_rand_base_ns += (now_ns() - t_rb0);
        g_prof.mr_rand_bases++;
#endif

        // 防御：确保 2 <= base <= n-2
        if (bn_cmp(base, BN_TWO()) < 0) bn_from_int(base, 2);
        if (bn_cmp(base, dec.n_minus_1) >= 0) { // base >= n-1，则减 2
            bn_sub(base, dec.n_minus_1, BN_TWO());
        }

        if (!mr_round(n, base, &dec, x))
        {
            bn_free(x);
            bn_free(base);
            bn_free(n_minus_3);
            mr_decompose_free(&dec);
            return false;
        }
        used_rounds++;
    }

    bn_free(x);
    bn_free(base);
    bn_free(n_minus_3);
    mr_decompose_free(&dec);
    return true;
}

/* ========== 综合素性测试 ========== */

bool prime_is_prime(const BigNum *n)
{
    // 基本检查
    if (bn_cmp(n, BN_TWO()) < 0) return false;
    if (bn_cmp(n, BN_TWO()) == 0) return true;
    if (bn_is_even(n)) return false;

    // 小素数试除（快速排除大多数合数）
    if (!prime_trial_division(n)) return false;

    // 若 n < 1000 且通过试除，直接判素
    BigNum *max_small = bn_new();
    bn_from_int(max_small, 1000);
    if (bn_cmp(n, max_small) < 0)
    {
        bn_free(max_small);
        return true;
    }
    bn_free(max_small);

    // Miller-Rabin（采用上面的 rounds 控制上限）
    return prime_is_probable_prime(n, PRIME_MILLER_RABIN_ROUNDS);
}

/* ========== 素数生成（轮筛 + 预筛 + MR） ========== */

static void candidate_add_small(BigNum *n, int delta)
{
    BigNum *d = bn_new();
    bn_from_int(d, delta);
    bn_add(n, n, d);
    bn_free(d);
}

BigNum *prime_generate(int bits)
{
    BigNum *result = bn_new();
    prime_generate_ex(result, bits);
    return result;
}

int prime_generate_ex(BigNum *result, int bits)
{
    init_random();

    if (bits < 2)
    {
        fprintf(stderr, "Error: bits must be >= 2\n");
        bn_from_int(result, 2);
        return 0;
    }

#if PRIME_PROFILE
    prof_reset();
    uint64_t t_total0 = now_ns();
#endif

    int candidates_tested = 0;

    BigNum *candidate = bn_new();

    // 小常量步进
    BigNum *B2 = bn_new(); bn_from_int(B2, 2);
    BigNum *B4 = bn_new(); bn_from_int(B4, 4);
    BigNum *B6 = bn_new(); bn_from_int(B6, 6);

    // 生成初始随机奇数候选
    generate_random_odd(candidate, bits);

    // 轮筛对齐：将 candidate 调整到 mod 30 的允许余数之一
#if PRIME_PROFILE
    uint64_t t_align0 = now_ns();
#endif
    int r = (int)bn_mod_u32(candidate, 30);
    int idx = wheel_residue_index(r);
    if (idx < 0)
    {
        // 找到下一个允许余数并前移
        for (int step = 1; step <= 30; ++step)
        {
            int nr = (r + step) % 30;
            int nidx = wheel_residue_index(nr);
            if (nidx >= 0)
            {
                candidate_add_small(candidate, step);
                idx = nidx;
                break;
            }
        }
        if (idx < 0) idx = 0; // 极端情况兜底
    }
#if PRIME_PROFILE
    g_prof.wheel_align_ns += (now_ns() - t_align0);
#endif

    // 主循环：步进遵循 WHEEL_DELTAS，避免触及 2/3/5 的倍数
    while (true)
    {
        candidates_tested++;
#if PRIME_PROFILE
        g_prof.candidates_tested = (uint64_t)candidates_tested;
#endif

        // 试除法（u32 加速）
        if (!prime_trial_division(candidate))
        {
            // 按轮筛步进
#if PRIME_PROFILE
            uint64_t t_step0 = now_ns();
#endif
            int delta = WHEEL_DELTAS[idx];
            if (delta == 2) bn_add(candidate, candidate, B2);
            else if (delta == 4) bn_add(candidate, candidate, B4);
            else bn_add(candidate, candidate, B6);
            idx = (idx + 1) % 8;
#if PRIME_PROFILE
            g_prof.wheel_step_ns += (now_ns() - t_step0);
#endif
            continue;
        }

        // Miller-Rabin（受控轮数上限）
        if (!prime_is_probable_prime(candidate, PRIME_MILLER_RABIN_ROUNDS))
        {
            int delta = WHEEL_DELTAS[idx];
#if PRIME_PROFILE
            uint64_t t_step0 = now_ns();
#endif
            if (delta == 2) bn_add(candidate, candidate, B2);
            else if (delta == 4) bn_add(candidate, candidate, B4);
            else bn_add(candidate, candidate, B6);
            idx = (idx + 1) % 8;
#if PRIME_PROFILE
            g_prof.wheel_step_ns += (now_ns() - t_step0);
#endif
            continue;
        }

        // 找到素数：复制结果
        if (result->data)
        {
            free(result->data);
        }
        result->data = (uint32_t *)malloc(candidate->capacity * sizeof(uint32_t));
        memcpy(result->data, candidate->data, candidate->length * sizeof(uint32_t));
        result->length = candidate->length;
        result->capacity = candidate->capacity;
        result->sign = candidate->sign;

        bn_free(candidate);
        bn_free(B2); bn_free(B4); bn_free(B6);

#if PRIME_PROFILE
        g_prof.total_ns = now_ns() - t_total0;
        prof_dump("prime_generate_ex", bits, candidates_tested);
#endif
        return candidates_tested;
    }
}

/* ========== 统计函数 ========== */

void prime_reset_stats(void)
{
    memset(&g_stats, 0, sizeof(PrimeGenStats));
}

PrimeGenStats prime_get_stats(void)
{
    return g_stats;
}
