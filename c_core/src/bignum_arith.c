
/**
 * bignum_arith.c - 大数加减乘除运算
 */

#include "bignum.h"
#include <string.h>
#include <stdio.h>

/* ========== 内部辅助函数 ========== */

// 绝对值加法：result = |a| + |b|
static void bn_add_abs(BigNum *result, const BigNum *a, const BigNum *b)
{
    if (result == a || result == b)
    {
        BigNum *temp = bn_new();
        bn_add_abs(temp, a, b);

        // 复制结果回 result
        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL; // 防止 bn_free 释放数据
        bn_free(temp);
        return;
    }
    int max_len = (a->length > b->length) ? a->length : b->length;

    if (result->capacity < max_len + 1)
    {
        bn_resize(result, max_len + 1);
    }

    uint64_t carry = 0;
    for (int i = 0; i < max_len || carry; i++)
    {
        uint64_t sum = carry;
        if (i < a->length)
            sum += a->data[i];
        if (i < b->length)
            sum += b->data[i];

        result->data[i] = (uint32_t)sum;
        carry = sum >> 32;
    }

    result->length = max_len;
    if (carry)
    {
        result->data[result->length++] = (uint32_t)carry;
    }

    bn_cleanup(result);
}

// 绝对值减法：result = |a| - |b|，假设 |a| >= |b|
static void bn_sub_abs(BigNum *result, const BigNum *a, const BigNum *b)
{
    if (result->capacity < a->length)
    {
        bn_resize(result, a->length);
    }

    int64_t borrow = 0;
    for (int i = 0; i < a->length; i++)
    {
        int64_t diff = (int64_t)a->data[i] - borrow;
        if (i < b->length)
        {
            diff -= b->data[i];
        }

        if (diff < 0)
        {
            diff += (1ULL << 32);
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }

        result->data[i] = (uint32_t)diff;
    }

    result->length = a->length;
    bn_cleanup(result);
}

/* ========== 加法 ========== */

void bn_add(BigNum *result, const BigNum *a, const BigNum *b)
{
    if (result == a || result == b)
    {
        BigNum *temp = bn_new();
        bn_add(temp, a, b);

        // 复制结果回 result
        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL; // 防止 bn_free 释放数据
        bn_free(temp);
        return;
    }

    if (result->capacity < (a->length > b->length ? a->length : b->length) + 1)
    {
        bn_resize(result, (a->length > b->length ? a->length : b->length) + 1);
    }
    // 处理零
    if (bn_is_zero(a))
    {
        memcpy(result->data, b->data, b->length * sizeof(uint32_t));
        result->length = b->length;
        result->sign = b->sign;
        return;
    }
    if (bn_is_zero(b))
    {
        memcpy(result->data, a->data, a->length * sizeof(uint32_t));
        result->length = a->length;
        result->sign = a->sign;
        return;
    }

    // 同号：绝对值相加
    if (a->sign == b->sign)
    {
        bn_add_abs(result, a, b);
        result->sign = a->sign;
    }
    // 异号：绝对值相减
    else
    {
        int cmp = bn_cmp_abs(a, b);
        if (cmp == 0)
        {
            bn_clear(result);
        }
        else if (cmp > 0)
        {
            bn_sub_abs(result, a, b);
            result->sign = a->sign;
        }
        else
        {
            bn_sub_abs(result, b, a);
            result->sign = b->sign;
        }
    }
}

/* ========== 减法 ========== */

void bn_sub(BigNum *result, const BigNum *a, const BigNum *b)
{
    if (result == a || result == b)
    {
        BigNum *temp = bn_new();
        bn_sub(temp, a, b);

        // 复制结果回 result
        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL;
        bn_free(temp);
        return;
    }

    // a - b = a + (-b)
    int orig_sign = b->sign;
    ((BigNum *)b)->sign = -b->sign;
    bn_add(result, a, b);
    ((BigNum *)b)->sign = orig_sign;
}

/* ========== 乘法（朴素算法和Karatsuba优化）========== */
void bn_mul_naive(BigNum *result, const BigNum *a, const BigNum *b)
{
    // 处理零
    if (bn_is_zero(a) || bn_is_zero(b))
    {
        bn_clear(result);
        return;
    }

    int result_len = a->length + b->length + 1;
    BigNum *temp = bn_new_size(result_len);
    memset(temp->data, 0, result_len * sizeof(uint32_t));

    // 朴素乘法 O(n^2)
    for (int i = 0; i < a->length; i++)
    {
        uint64_t carry = 0;
        for (int j = 0; j < b->length || carry; j++)
        {
            uint64_t prod = carry;
            if (j < b->length)
            {
                prod += (uint64_t)a->data[i] * b->data[j];
            }
            prod += temp->data[i + j];

            temp->data[i + j] = (uint32_t)prod;
            carry = prod >> 32;
        }
    }

    temp->length = result_len;
    temp->sign = a->sign * b->sign;
    bn_cleanup(temp);

    // 复制结果
    bn_resize(result, temp->length);
    memcpy(result->data, temp->data, temp->length * sizeof(uint32_t));
    result->length = temp->length;
    result->sign = temp->sign;

    bn_free(temp);
}

void bn_mul(BigNum *result, const BigNum *a, const BigNum *b)
{
    if (result == a || result == b)
    {
        BigNum *temp = bn_new();
        bn_mul(temp, a, b);

        // 复制结果回 result
        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL; // 防止 bn_free 释放数据
        bn_free(temp);
        return;
    }
    // 如果任一操作数为零，结果为零
    if (bn_is_zero(a) || bn_is_zero(b))
    {
        bn_clear(result);
        return;
    }

    // 选择较小的长度作为分割基准
    int min_len = (a->length < b->length) ? a->length : b->length;

    // 确保两个数字的长度都超过阈值才使用Karatsuba
    if (min_len <= BN_MUL_KARATSUBA_THRESHOLD)
    {
        bn_mul_naive(result, a, b);
        return;
    }

    // 分割点
    int m = min_len / 2;

    // 调试信息
    // printf("Karatsuba: a_len=%d, b_len=%d, m=%d\n", a->length, b->length, m);

    // 分配临时变量
    BigNum *a_low = bn_new_size(m + 2);
    BigNum *a_high = bn_new_size((a->length > m) ? (a->length - m + 2) : 2);
    BigNum *b_low = bn_new_size(m + 2);
    BigNum *b_high = bn_new_size((b->length > m) ? (b->length - m + 2) : 2);

    // 初始化并拆分数字
    bn_clear(a_low);
    bn_clear(a_high);
    bn_clear(b_low);
    bn_clear(b_high);

    // 拆分 a - 使用 memcpy 提高效率
    if (a->length > 0)
    {
        int a_low_len = (a->length < m) ? a->length : m;
        if (a_low_len > 0)
        {
            memcpy(a_low->data, a->data, a_low_len * sizeof(uint32_t));
            a_low->length = a_low_len;
        }

        int a_high_len = (a->length > m) ? (a->length - m) : 1;
        if (a_high_len > 0 && a->length > m)
        {
            memcpy(a_high->data, a->data + m, a_high_len * sizeof(uint32_t));
            a_high->length = a_high_len;
        }
    }
    a_low->sign = 1;
    a_high->sign = 1;

    // 拆分 b - 使用 memcpy 提高效率
    if (b->length > 0)
    {
        int b_low_len = (b->length < m) ? b->length : m;
        if (b_low_len > 0)
        {
            memcpy(b_low->data, b->data, b_low_len * sizeof(uint32_t));
            b_low->length = b_low_len;
        }

        int b_high_len = (b->length > m) ? (b->length - m) : 1;
        if (b_high_len > 0 && b->length > m)
        {
            memcpy(b_high->data, b->data + m, b_high_len * sizeof(uint32_t));
            b_high->length = b_high_len;
        }
    }
    b_low->sign = 1;
    b_high->sign = 1;

    // 清理前导零
    bn_cleanup(a_low);
    bn_cleanup(a_high);
    bn_cleanup(b_low);
    bn_cleanup(b_high);

    // 计算三个乘积
    BigNum *z0 = bn_new(); // z0 = a_low * b_low
    BigNum *z2 = bn_new(); // z2 = a_high * b_high
    BigNum *z1 = bn_new(); // z1 = (a_low + a_high) * (b_low + b_high)

    // 递归计算乘积
    bn_mul(z0, a_low, b_low);
    bn_mul(z2, a_high, b_high);

    // 计算中间和
    BigNum *a_sum = bn_new();
    BigNum *b_sum = bn_new();

    bn_add(a_sum, a_low, a_high);
    bn_add(b_sum, b_low, b_high);

    // 递归计算 z1
    bn_mul(z1, a_sum, b_sum);

    // 计算 z1 = z1 - z0 - z2
    BigNum *temp = bn_new();
    bn_sub(temp, z1, z0);
    bn_sub(z1, temp, z2);

    // 组合结果
    bn_resize(result, a->length + b->length + 4);
    bn_clear(result);

    // 添加 z0
    bn_add(result, result, z0);

    // 添加 z1 * 2^(32*m)
    if (!bn_is_zero(z1))
    {
        BigNum *z1_shifted = bn_new();
        bn_lshift(z1_shifted, z1, 32 * m);
        bn_add(result, result, z1_shifted);
        bn_free(z1_shifted);
    }

    // 添加 z2 * 2^(64*m)
    if (!bn_is_zero(z2))
    {
        BigNum *z2_shifted = bn_new();
        bn_lshift(z2_shifted, z2, 64 * m);
        bn_add(result, result, z2_shifted);
        bn_free(z2_shifted);
    }

    // 设置符号
    result->sign = a->sign * b->sign;
    bn_cleanup(result);

    // 清理临时变量
    bn_free(a_low);
    bn_free(a_high);
    bn_free(b_low);
    bn_free(b_high);
    bn_free(z0);
    bn_free(z1);
    bn_free(z2);
    bn_free(a_sum);
    bn_free(b_sum);
    bn_free(temp);
}
/* ========== 除法（长除法算法）========== */
void bn_div(BigNum *quotient, BigNum *remainder,
            const BigNum *a, const BigNum *b)
{
    if (bn_is_zero(b))
    {
        fprintf(stderr, "Error: Division by zero\n");
        if (quotient)
            bn_clear(quotient);
        if (remainder)
            bn_clear(remainder);
        return;
    }

    if (bn_is_zero(a))
    {
        if (quotient)
            bn_clear(quotient);
        if (remainder)
            bn_clear(remainder);
        return;
    }

    int cmp = bn_cmp_abs(a, b);
    if (cmp < 0)
    {
        if (quotient)
            bn_clear(quotient);
        if (remainder)
        {
            if (remainder->capacity < a->length)
                bn_resize(remainder, a->length);
            memcpy(remainder->data, a->data, a->length * sizeof(uint32_t));
            remainder->length = a->length;
            remainder->sign = a->sign;
        }
        return;
    }

    if (cmp == 0)
    {
        if (quotient)
        {
            bn_from_int(quotient, 1);
            quotient->sign = a->sign * b->sign;
        }
        if (remainder)
            bn_clear(remainder);
        return;
    }

    BigNum *q = quotient ? quotient : bn_new();
    BigNum *r = remainder ? remainder : bn_new();
    bn_clear(q);
    bn_clear(r);

    int bits = bn_bit_length(a);

    for (int i = bits - 1; i >= 0; i--)
    {
        // r <<= 1
        bn_lshift(r, r, 1);

        // r的最低位设置为a的第i位
        if (bn_get_bit(a, i))
            bn_set_bit(r, 0);

        // 如果 r >= b，则 r -= b，商第i位置1
        if (bn_cmp_abs(r, b) >= 0)
        {
            bn_sub_abs(r, r, b);
            bn_set_bit(q, i);
        }
    }

    if (!bn_is_zero(q))
        q->sign = a->sign * b->sign;
    if (!bn_is_zero(r))
        r->sign = a->sign;

    if (!quotient)
        bn_free(q);
    if (!remainder)
        bn_free(r);
}

/* ========== 模运算 ========== */
void bn_mod(BigNum *result, const BigNum *a, const BigNum *b)
{
    if (result == a || result == b)
    {
        BigNum *temp = bn_new();
        bn_mod(temp, a, b);

        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL;
        bn_free(temp);
        return;
    }

    bn_div(NULL, result, a, b);

    // 确保余数为正
    if (result->sign < 0)
    {
        bn_add(result, result, b);
    }
}

void bn_mod_add(BigNum *result, const BigNum *a, const BigNum *b,
                const BigNum *mod)
{
    if (result == a || result == b || result == mod)
    {
        BigNum *temp = bn_new();
        bn_mod_add(temp, a, b, mod);

        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL;
        bn_free(temp);
        return;
    }

    bn_add(result, a, b);
    bn_mod(result, result, mod);
}

void bn_mod_sub(BigNum *result, const BigNum *a, const BigNum *b,
                const BigNum *mod)
{
    if (result == a || result == b || result == mod)
    {
        BigNum *temp = bn_new();
        bn_mod_sub(temp, a, b, mod);

        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL;
        bn_free(temp);
        return;
    }

    bn_sub(result, a, b);
    if (result->sign < 0)
    {
        bn_add(result, result, mod);
    }
    bn_mod(result, result, mod);
}

void bn_mod_mul(BigNum *result, const BigNum *a, const BigNum *b,
                const BigNum *mod)
{
    if (result == a || result == b || result == mod)
    {
        BigNum *temp = bn_new();
        bn_mod_mul(temp, a, b, mod);

        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL;
        bn_free(temp);
        return;
    }

    bn_mul(result, a, b);
    bn_mod(result, result, mod);
}

/* ========== Montgomery 模乘辅助函数 ========== */

// Montgomery 参数结构
typedef struct
{
    BigNum *R;       // R = 2^(32 * mod->length)
    BigNum *R2;      // R^2 mod mod
    uint32_t n0_inv; // -mod^(-1) mod 2^32
    int n;           // mod的长度（字数）
} MontgomeryCtx;

// 计算 -n^(-1) mod 2^32（用于Montgomery算法）
static uint32_t compute_n0_inv(uint32_t n0)
{
    // 使用牛顿迭代法: x_{i+1} = x_i * (2 - n0 * x_i)
    uint32_t x = n0; // 初始近似

    // 迭代5次（对于32位足够）
    for (int i = 0; i < 5; i++)
    {
        x = x * (2 - n0 * x);
    }

    return ~x + 1; // 取负
}

// 初始化 Montgomery 上下文
static MontgomeryCtx *montgomery_init(const BigNum *mod)
{
    if (bn_is_zero(mod) || (mod->data[0] & 1) == 0)
    {
        return NULL; // Montgomery 算法要求模数为奇数
    }

    MontgomeryCtx *ctx = (MontgomeryCtx *)malloc(sizeof(MontgomeryCtx));
    ctx->n = mod->length;

    // 计算 n0_inv = -mod^(-1) mod 2^32
    ctx->n0_inv = compute_n0_inv(mod->data[0]);

    // 计算 R = 2^(32 * n)
    ctx->R = bn_new();
    bn_from_int(ctx->R, 1);
    bn_lshift(ctx->R, ctx->R, 32 * ctx->n);

    // 计算 R^2 mod mod
    ctx->R2 = bn_new();
    bn_mul(ctx->R2, ctx->R, ctx->R);
    bn_mod(ctx->R2, ctx->R2, mod);

    return ctx;
}

// 释放 Montgomery 上下文
static void montgomery_free(MontgomeryCtx *ctx)
{
    if (ctx)
    {
        bn_free(ctx->R);
        bn_free(ctx->R2);
        free(ctx);
    }
}

// Montgomery 约简: result = (a * R^(-1)) mod mod
// 假设 a < mod * R
static void montgomery_reduce(BigNum *result, const BigNum *a,
                              const BigNum *mod, MontgomeryCtx *ctx)
{
    int n = ctx->n;

    // 创建工作空间（避免重复分配）
    BigNum *t = bn_new_size(2 * n + 2);

    // 复制 a 到 t
    if (t->capacity < a->length)
    {
        bn_resize(t, a->length);
    }
    memcpy(t->data, a->data, a->length * sizeof(uint32_t));
    t->length = a->length;

    // Montgomery 约简算法
    for (int i = 0; i < n; i++)
    {
        // m = (t[i] * n0_inv) mod 2^32
        uint32_t m = t->data[i] * ctx->n0_inv;

        // t = t + m * mod * 2^(32*i)
        uint64_t carry = 0;
        for (int j = 0; j < mod->length; j++)
        {
            uint64_t prod = (uint64_t)m * mod->data[j] + carry;
            if (i + j < t->length)
            {
                prod += t->data[i + j];
            }
            else
            {
                // 扩展 t
                if (t->length <= i + j)
                {
                    if (t->capacity <= i + j)
                    {
                        bn_resize(t, i + j + 1);
                    }
                    t->length = i + j + 1;
                }
            }

            if (i + j >= t->capacity)
            {
                bn_resize(t, i + j + 1);
            }
            t->data[i + j] = (uint32_t)prod;
            carry = prod >> 32;
        }

        // 传播进位
        int k = i + mod->length;
        while (carry)
        {
            if (k >= t->length)
            {
                if (k >= t->capacity)
                {
                    bn_resize(t, k + 1);
                }
                t->data[k] = 0;
                t->length = k + 1;
            }
            uint64_t sum = (uint64_t)t->data[k] + carry;
            t->data[k] = (uint32_t)sum;
            carry = sum >> 32;
            k++;
        }
        if (k > t->length)
            t->length = k;
    }

    // 右移 n 个字（除以 R）
    if (t->length > n)
    {
        memmove(t->data, t->data + n, (t->length - n) * sizeof(uint32_t));
        t->length -= n;
    }
    else
    {
        t->length = 1;
        t->data[0] = 0;
    }

    bn_cleanup(t);

    // 如果 t >= mod，则 t = t - mod
    if (bn_cmp_abs(t, mod) >= 0)
    {
        bn_sub_abs(t, t, mod);
    }

    // 复制结果
    if (result->capacity < t->length)
    {
        bn_resize(result, t->length);
    }
    memcpy(result->data, t->data, t->length * sizeof(uint32_t));
    result->length = t->length;
    result->sign = 1;

    bn_free(t);
}

// Montgomery 模乘: result = (a * b * R^(-1)) mod mod
static void montgomery_mul(BigNum *result, const BigNum *a, const BigNum *b,
                           const BigNum *mod, MontgomeryCtx *ctx)
{
    // t = a * b
    BigNum *t = bn_new();
    bn_mul(t, a, b);

    // result = montgomery_reduce(t)
    montgomery_reduce(result, t, mod, ctx);

    bn_free(t);
}

/* ========== 优化的模幂运算（Montgomery + 滑动窗口）========== */
#define WINDOW 4
#define TABLE_SIZE (1 << (WINDOW - 1))

void bn_mod_exp(BigNum *result, const BigNum *base, const BigNum *exp,
                const BigNum *mod)
{
    if (result == base || result == exp || result == mod)
    {
        BigNum *temp = bn_new();
        bn_mod_exp(temp, base, exp, mod);

        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;

        temp->data = NULL;
        bn_free(temp);
        return;
    }

    // 特殊情况处理
    if (bn_is_zero(exp))
    {
        bn_from_int(result, 1);
        return;
    }

    if (bn_is_zero(mod) || (mod->length == 1 && mod->data[0] == 1))
    {
        bn_clear(result);
        return;
    }

    if (bn_is_zero(base))
    {
        bn_clear(result);
        return;
    }

    // 对于小模数或偶数模数，使用原始算法
    if (mod->length <= 2 || (mod->data[0] & 1) == 0)
    {
        // 使用原始的平方-乘算法（作为fallback）
        BigNum *temp_base = bn_copy(base);
        bn_mod(temp_base, temp_base, mod);

        BigNum *temp_result = bn_new();
        bn_from_int(temp_result, 1);

        int bits = bn_bit_length(exp);
        for (int i = 0; i < bits; i++)
        {
            if (bn_get_bit(exp, i))
            {
                BigNum *mul_temp = bn_new();
                bn_mul(mul_temp, temp_result, temp_base);
                bn_mod(temp_result, mul_temp, mod);
                bn_free(mul_temp);
            }

            if (i < bits - 1)
            {
                BigNum *sqr_temp = bn_new();
                bn_mul(sqr_temp, temp_base, temp_base);
                bn_mod(temp_base, sqr_temp, mod);
                bn_free(sqr_temp);
            }
        }

        if (result->capacity < temp_result->length)
        {
            bn_resize(result, temp_result->length);
        }
        memcpy(result->data, temp_result->data, temp_result->length * sizeof(uint32_t));
        result->length = temp_result->length;
        result->sign = temp_result->sign;

        bn_free(temp_base);
        bn_free(temp_result);
        return;
    }

    // ========== 使用 Montgomery 算法 ==========

    MontgomeryCtx *ctx = montgomery_init(mod);
    if (!ctx)
    {
        // 失败则使用原始算法（不应该发生，因为已检查奇数）
        fprintf(stderr, "Warning: Montgomery init failed\n");
        return;
    }

    // 转换 base 到 Montgomery 域: base_mont = (base * R) mod mod
    BigNum *base_norm = bn_new();
    bn_mod(base_norm, base, mod);

    BigNum *base_mont = bn_new();
    bn_mul(base_mont, base_norm, ctx->R);
    bn_mod(base_mont, base_mont, mod);

    // 使用4位滑动窗口 WINDOW

    // 预计算表: table[i] = base^(2i+1) 在 Montgomery 域中
    BigNum *table[TABLE_SIZE];
    table[0] = bn_copy(base_mont); // base^1

    // base^2 在 Montgomery 域中
    BigNum *base2_mont = bn_new();
    montgomery_mul(base2_mont, base_mont, base_mont, mod, ctx);

    // 预计算奇数幂次
    for (int i = 1; i < TABLE_SIZE; i++)
    {
        table[i] = bn_new();
        montgomery_mul(table[i], table[i - 1], base2_mont, mod, ctx);
    }

    // 初始化结果为 1 在 Montgomery 域中: result_mont = R mod mod
    BigNum *result_mont = bn_new();
    bn_mod(result_mont, ctx->R, mod);

    // 从最高位开始扫描指数（从左到右）
    int bits = bn_bit_length(exp);
    int i = bits - 1;

    while (i >= 0)
    {
        // 如果当前位是0，平方一次
        if (!bn_get_bit(exp, i))
        {
            montgomery_mul(result_mont, result_mont, result_mont, mod, ctx);
            i--;
        }
        else
        {
            // 找到窗口：读取最多 WINDOW 位
            int window_val = 0;
            int window_len = 0;

            while (window_len < WINDOW && i >= 0)
            {
                window_val = (window_val << 1) | bn_get_bit(exp, i);
                window_len++;
                i--;

                // 如果遇到0，或者是最后一位，停止
                if (window_val > 0 && i >= 0 && !bn_get_bit(exp, i))
                {
                    break;
                }
            }

            // 平方 window_len 次
            for (int j = 0; j < window_len; j++)
            {
                montgomery_mul(result_mont, result_mont, result_mont, mod, ctx);
            }

            // 乘以预计算的值
            if (window_val > 0)
            {
                // window_val 应该是奇数，找到对应的表项
                // 去掉尾部的0
                int shift = 0;
                while ((window_val & 1) == 0)
                {
                    window_val >>= 1;
                    shift++;
                }

                // 额外的平方
                for (int j = 0; j < shift; j++)
                {
                    montgomery_mul(result_mont, result_mont, result_mont, mod, ctx);
                }

                // 乘以 base^window_val
                int table_idx = (window_val - 1) / 2;
                if (table_idx < TABLE_SIZE)
                {
                    montgomery_mul(result_mont, result_mont, table[table_idx], mod, ctx);
                }
            }
        }
    }

    // 转换回普通域: result = result_mont * R^(-1) mod mod
    montgomery_reduce(result, result_mont, mod, ctx);

    // 清理
    bn_free(base_norm);
    bn_free(base_mont);
    bn_free(base2_mont);
    bn_free(result_mont);
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        bn_free(table[i]);
    }
    montgomery_free(ctx);
}

/* ========== 扩展欧几里得算法（求模逆）========== */
void bn_mod_inverse(BigNum *result, const BigNum *a, const BigNum *mod)
{
    if (result == a || result == mod)
    {
        BigNum *temp = bn_new();
        bn_mod_inverse(temp, a, mod);
        if (result->data)
            free(result->data);
        result->data = temp->data;
        result->length = temp->length;
        result->capacity = temp->capacity;
        result->sign = temp->sign;
        temp->data = NULL;
        bn_free(temp);
        return;
    }

    BigNum *old_r = bn_copy(a), *r = bn_copy(mod);
    BigNum *old_x = bn_new(), *x = bn_new(), *old_y = bn_new(), *y = bn_new();
    bn_from_int(old_x, 1);
    bn_from_int(x, 0);
    bn_from_int(old_y, 0);
    bn_from_int(y, 1);

    BigNum *zero = bn_new();
    bn_from_int(zero, 0);
    BigNum *q = bn_new(), *tmp = bn_new();

    while (bn_cmp(r, zero))
    {
        bn_div(q, tmp, old_r, r);
        bn_free(old_r);
        old_r = r;
        r = tmp;
        tmp = bn_new();

        bn_mod_mul(tmp, q, x, mod);
        bn_mod_sub(tmp, old_x, tmp, mod);
        bn_free(old_x);
        old_x = x;
        x = tmp;
        tmp = bn_new();

        bn_mod_mul(tmp, q, y, mod);
        bn_mod_sub(tmp, old_y, tmp, mod);
        bn_free(old_y);
        old_y = y;
        y = tmp;
        tmp = bn_new();
    }

    if(old_r->sign < 0) {
        old_x->sign = old_x->sign * (-1);
    }

    bn_clear(result);
    bn_mod(result, old_x, mod);

    bn_free(old_r);
    bn_free(r);
    bn_free(old_x);
    bn_free(x);
    bn_free(old_y);
    bn_free(y);
    bn_free(zero);
    bn_free(q);
    bn_free(tmp);
}

/* ========== 随机数生成 ========== */

void bn_rand(BigNum *bn, int bits)
{
    if (bits <= 0)
    {
        bn_clear(bn);
        return;
    }

    int words = (bits + 31) / 32;
    if (bn->capacity < words)
    {
        bn_resize(bn, words);
    }

    // 生成随机数据
    for (int i = 0; i < words; i++)
    {
        bn->data[i] = ((uint32_t)rand() << 16) | (uint32_t)rand();
    }

    // 清除多余的高位
    int extra_bits = bits % 32;
    if (extra_bits > 0)
    {
        bn->data[words - 1] &= (1U << extra_bits) - 1;
    }

    // 确保最高位为1（生成完整bits位的数）
    bn_set_bit(bn, bits - 1);

    bn->length = words;
    bn->sign = 1;
    bn_cleanup(bn);
}

void bn_rand_range(BigNum *bn, const BigNum *max)
{
    if (bn_is_zero(max))
    {
        bn_clear(bn);
        return;
    }

    int bits = bn_bit_length(max);

    // 生成一个不超过max的随机数
    do
    {
        bn_rand(bn, bits);
    } while (bn_cmp(bn, max) >= 0);
}
