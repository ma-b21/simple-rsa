/**
 * bignum.c - 大数基础运算实现
 */

#include "bignum.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

/* ========== 内存管理 ========== */

BigNum *bn_new(void)
{
    return bn_new_size(1);
}

BigNum *bn_new_size(int capacity)
{
    BigNum *bn = malloc(sizeof(BigNum));
    if (!bn)
        return NULL;

    bn->data = calloc(capacity, sizeof(uint32_t));
    if (!bn->data)
    {
        free(bn);
        return NULL;
    }

    bn->length = 1;
    bn->capacity = capacity;
    bn->sign = 0; // 初始为0
    return bn;
}

void bn_free(BigNum *bn)
{
    if (bn)
    {
        free(bn->data);
        free(bn);
    }
}

BigNum *bn_copy(const BigNum *bn)
{
    BigNum *copy = bn_new_size(bn->capacity);
    if (!copy)
        return NULL;

    memcpy(copy->data, bn->data, bn->length * sizeof(uint32_t));
    copy->length = bn->length;
    copy->sign = bn->sign;
    return copy;
}

void bn_resize(BigNum *bn, int new_capacity)
{
    if (new_capacity <= bn->capacity)
        return;

    uint32_t *new_data = realloc(bn->data, new_capacity * sizeof(uint32_t));
    if (!new_data)
        return;

    // 清零新分配的空间
    memset(new_data + bn->capacity, 0,
           (new_capacity - bn->capacity) * sizeof(uint32_t));

    bn->data = new_data;
    bn->capacity = new_capacity;
}

void bn_clear(BigNum *bn)
{
    memset(bn->data, 0, bn->capacity * sizeof(uint32_t));
    bn->length = 1;
    bn->sign = 0;
}

void bn_cleanup(BigNum *bn)
{
    // 移除前导零
    while (bn->length > 1 && bn->data[bn->length - 1] == 0)
    {
        bn->length--;
    }
    if (bn->length == 0)
    {
        bn->length = 1;
        bn->data[0] = 0;
    }

    // 如果结果为0，符号设为0
    if (bn->length == 1 && bn->data[0] == 0)
    {
        bn->sign = 0;
    }
}

/* ========== 初始化和转换 ========== */

void bn_from_int(BigNum *bn, uint64_t val)
{
    bn_clear(bn);
    if (val == 0)
    {
        bn->sign = 0;
        return;
    }

    if (bn->capacity < 2)
        bn_resize(bn, 2);

    bn->data[0] = (uint32_t)(val & 0xFFFFFFFF);
    bn->data[1] = (uint32_t)(val >> 32);
    bn->length = (bn->data[1] == 0) ? 1 : 2;
    bn->sign = 1;
}

void bn_from_hex(BigNum *bn, const char *hex)
{
    bn_clear(bn);
    bn->sign = (hex[0] == '-') ? -1 : 1;
    if( bn->sign == -1)
    {
        hex++; // 跳过负号
    }

    // 跳过"0x"前缀
    if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
    {
        hex += 2;
    }

    int hex_len = (int)strlen(hex);
    if (hex_len == 0)
        return;

    // 每8个十六进制字符 = 1个uint32_t
    int needed = (hex_len + 7) / 8;
    if (bn->capacity < needed)
    {
        bn_resize(bn, needed);
    }

    int idx = 0;
    int pos = hex_len;

    while (pos > 0)
    {
        int chunk_size = (pos >= 8) ? 8 : pos;
        pos -= chunk_size;

        char chunk[9];
        memcpy(chunk, hex + pos, chunk_size);
        chunk[chunk_size] = '\0';

        bn->data[idx++] = (uint32_t)strtoul(chunk, NULL, 16);
    }

    bn->length = idx;
    bn_cleanup(bn);
}

char *bn_to_hex(const BigNum *bn)
{
    if (bn->sign == 0)
    {
        char *result = malloc(2);
        strcpy_s(result, 1, "0");
        return result;
    }

    // 计算需要的字符数：每个uint32_t最多8个hex字符
    int max_len = bn->length * 8 + 4; // +3 for "0x" and '\0' and '-'
    char *result = malloc(max_len);

    int pos = 0;
    if(bn->sign < 0){
        result[pos++] = '-';
    }
    result[pos++] = '0';
    result[pos++] = 'x';

    // 最高位不需要前导零
    pos += sprintf_s(result + pos, max_len - pos,"%x", bn->data[bn->length - 1]);

    // 其余位需要补齐8位
    for (int i = bn->length - 2; i >= 0; i--)
    {
        pos += sprintf_s(result + pos, max_len - pos, "%08x", bn->data[i]);
    }
    result[pos] = '\0';

    return result;
}

void bn_from_bytes(BigNum *bn, const uint8_t *bytes, size_t len)
{
    bn_clear(bn);
    if (len == 0)
        return;

    // 每 4 个字节 = 1 个 uint32_t
    int needed = (int)(len + 3) / 4;
    if (bn->capacity < needed)
        bn_resize(bn, needed);

    int idx = 0;
    int rem = len % 4;
    size_t pos = 0;

    // 先处理最前面的不足 4 字节
    if (rem != 0)
    {
        uint32_t word = 0;
        for (int i = 0; i < rem; i++)
        {
            word = (word << 8) | bytes[pos++];
        }
        bn->data[idx++] = word;
    }

    // 处理剩余整 4 字节
    while (pos < len)
    {
        uint32_t word = 0;
        for (int i = 0; i < 4; i++)
        {
            word = (word << 8) | bytes[pos++];
        }
        bn->data[idx++] = word;
    }

    bn->length = idx;
    bn->sign = 1;
    bn_cleanup(bn);
}

uint64_t bn_to_uint64(const BigNum *bn)
{
    if (bn->sign == 0)
        return 0;

    uint64_t result = bn->data[0];
    if (bn->length > 1)
    {
        result |= ((uint64_t)bn->data[1] << 32);
    }
    return result;
}

/* ========== 比较操作 ========== */
bool bn_is_zero(const BigNum *bn)
{
    if (!bn)
        return true;
    // 若长度==1 且 data[0]==0，则为零
    if (bn->length == 1 && bn->data[0] == 0)
        return true;
    // 保险起见，如果 sign==0 也认为是零
    if (bn->sign == 0)
        return true;
    return false;
}

bool bn_is_odd(const BigNum *bn)
{
    return !bn_is_zero(bn) && (bn->data[0] & 1);
}

bool bn_is_even(const BigNum *bn)
{
    return !bn_is_odd(bn);
}

int bn_cmp_abs(const BigNum *a, const BigNum *b)
{
    if (a->length != b->length)
    {
        return (a->length > b->length) ? 1 : -1;
    }

    for (int i = a->length - 1; i >= 0; i--)
    {
        if (a->data[i] != b->data[i])
        {
            return (a->data[i] > b->data[i]) ? 1 : -1;
        }
    }

    return 0;
}

int bn_cmp(const BigNum *a, const BigNum *b)
{
    // 处理零
    if (bn_is_zero(a) && bn_is_zero(b))
        return 0;
    if (bn_is_zero(a))
        return -b->sign;
    if (bn_is_zero(b))
        return a->sign;

    // 符号不同
    if (a->sign != b->sign)
    {
        return a->sign;
    }

    // 符号相同，比较绝对值
    int cmp = bn_cmp_abs(a, b);
    return (a->sign > 0) ? cmp : -cmp;
}

/* ========== 位操作 ========== */

int bn_bit_length(const BigNum *bn)
{
    if (bn_is_zero(bn))
        return 0;

    uint32_t top_word = bn->data[bn->length - 1];
    int bits = (bn->length - 1) * 32;

    // 计算最高字的位数
    while (top_word > 0)
    {
        bits++;
        top_word >>= 1;
    }

    return bits;
}

int bn_get_bit(const BigNum *bn, int pos)
{
    int word_idx = pos / 32;
    int bit_idx = pos % 32;

    if (word_idx >= bn->length)
        return 0;

    return (bn->data[word_idx] >> bit_idx) & 1;
}

void bn_set_bit(BigNum *bn, int pos)
{
    int word_idx = pos / 32;
    int bit_idx = pos % 32;

    if (word_idx >= bn->capacity)
    {
        bn_resize(bn, word_idx + 1);
    }

    bn->data[word_idx] |= (1U << bit_idx);

    if (word_idx >= bn->length)
    {
        bn->length = word_idx + 1;
    }

    if (bn->sign == 0)
        bn->sign = 1;
}

void bn_lshift(BigNum *result, const BigNum *bn, int bits)
{
    if(result == bn)
    {
        BigNum *temp = bn_copy(bn);
        bn_lshift(result, temp, bits);
        bn_free(temp);
        return;
    }
    if (!result || !bn)
        return;

    if (bits <= 0)
    {
        if (result != bn)
            bn_from_int(result, 0), memcpy(result->data, bn->data, bn->length * sizeof(uint32_t)),
                result->length = bn->length, result->sign = bn->sign;
        return;
    }

    if (bn_is_zero(bn))
    {
        bn_clear(result);
        return;
    }

    /* 若 result == bn，需要临时副本防止自毁 */
    const BigNum *src = bn;
    BigNum *tmp = NULL;
    if (result == bn)
    {
        tmp = bn_copy(bn); // 新分配副本
        src = tmp;
    }

    int word_shift = bits / 32;
    int bit_shift = bits % 32;
    int new_length = src->length + word_shift + (bit_shift ? 1 : 0);

    if (result->capacity < new_length)
        bn_resize(result, new_length);

    memset(result->data, 0, new_length * sizeof(uint32_t));

    uint32_t carry = 0;
    for (int i = 0; i < src->length; i++)
    {
        uint64_t val = ((uint64_t)src->data[i] << bit_shift) | carry;
        result->data[i + word_shift] = (uint32_t)val;
        carry = (uint32_t)(val >> 32);
    }

    if (carry)
        result->data[src->length + word_shift] = carry;

    result->length = new_length;
    result->sign = src->sign;
    bn_cleanup(result);

    if (tmp)
        bn_free(tmp);
}

void bn_rshift(BigNum *result, const BigNum *bn, int bits)
{
    if (!result || !bn)
        return;

    if (bits <= 0)
    {
        if (result != bn)
            bn_from_int(result, 0), memcpy(result->data, bn->data, bn->length * sizeof(uint32_t)),
                result->length = bn->length, result->sign = bn->sign;
        return;
    }

    if (bn_is_zero(bn))
    {
        bn_clear(result);
        return;
    }

    int word_shift = bits / 32;
    int bit_shift = bits % 32;

    if (word_shift >= bn->length)
    {
        bn_clear(result);
        return;
    }

    const BigNum *src = bn;
    BigNum *tmp = NULL;
    if (result == bn)
    {
        tmp = bn_copy(bn);
        src = tmp;
    }

    int new_length = src->length - word_shift;
    if (new_length <= 0)
    {
        bn_clear(result);
        if (tmp)
            bn_free(tmp);
        return;
    }

    if (result->capacity < new_length)
        bn_resize(result, new_length);

    for (int i = 0; i < new_length; ++i)
    {
        uint32_t low = src->data[i + word_shift];
        uint32_t high = (i + word_shift + 1 < src->length) ? src->data[i + word_shift + 1] : 0;
        if (bit_shift == 0)
            result->data[i] = low;
        else
            result->data[i] = (low >> bit_shift) | (high << (32 - bit_shift));
    }

    result->length = new_length;
    result->sign = src->sign;
    bn_cleanup(result);

    if (tmp)
        bn_free(tmp);
}

/* ========== 辅助函数 ========== */

void bn_print(const char *label, const BigNum *bn)
{
    printf("%s: ", label);
    if (bn->sign < 0)
        printf("-");
    if (bn_is_zero(bn))
    {
        printf("0\n");
        return;
    }

    printf("0x");
    for (int i = bn->length - 1; i >= 0; i--)
    {
        if (i == bn->length - 1)
        {
            printf("%x", bn->data[i]);
        }
        else
        {
            printf("%08x", bn->data[i]);
        }
    }
    printf(" (%d bits)\n", bn_bit_length(bn));
}