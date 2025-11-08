/**
 * test_rsa.c - RSA功能测试
 * 编译: gcc -o test_rsa test_rsa.c rsa.c prime.c bignum.c bignum_arith.c -lm
 */

#include "rsa.h"
#include "prime.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void test_init_and_conversion()
{
    printf("=== Test: Initialization & Conversion ===\n");
    BigNum *a = bn_new();
    bn_from_int(a, 123456789);
    bn_print("bn_from_int 123456789", a);

    bn_from_hex(a, "FFFFFFFF");
    bn_print("bn_from_hex 0xFFFFFFFF", a);

    uint64_t val = bn_to_uint64(a);
    printf("bn_to_uint64: %lu\n", val);

    char *hex_str = bn_to_hex(a);
    printf("bn_to_hex: %s\n", hex_str);
    free(hex_str);

    bn_free(a);
    printf("\n");
}

void test_add_sub_mul()
{
    printf("=== Test: Add, Sub, Mul ===\n");
    BigNum *a = bn_new();
    BigNum *b = bn_new();
    BigNum *r = bn_new();

    bn_from_int(a, 12345);
    bn_from_int(b, 67890);
    bn_add(r, a, b);
    bn_print("12345 + 67890", r);

    bn_sub(r, b, a);
    bn_print("67890 - 12345", r);

    bn_from_int(a, 123456789);
    bn_from_int(b, 567899999);
    bn_mul(r, a, b);
    bn_print("123456789 * 567899999", r);

    bn_free(a);
    bn_free(b);
    bn_free(r);
    printf("\n");
}

void test_div_mod()
{
    printf("=== Test: Div & Mod ===\n");
    BigNum *a = bn_new();
    BigNum *b = bn_new();
    BigNum *q = bn_new();
    BigNum *r = bn_new();

    bn_from_hex(a, "bdffcd5adee6126b7afc5d48dcf71de77ebf600f9cc79a596ce55a8ef5fe29c3");
    bn_from_hex(b, "7bdff07dacbe609a766905ee563d26f71bdec1c5bafff8f1eff611a3fcf4f601");
    bn_div(q, r, a, b);
    bn_print("12345 / 100 = quotient", q);
    bn_print("12345 / 100 = remainder", r);

    // 模运算测试
    bn_from_int(a, 3);
    bn_from_int(b, 7);
    bn_mod(r, a, b);
    bn_print("3 mod 7", r);

    // 更大数模幂
    BigNum *base = bn_new();
    BigNum *exp = bn_new();
    BigNum *mod = bn_new();
    BigNum *res = bn_new();

    bn_from_int(base, 3);
    bn_from_int(exp, 10);
    bn_from_int(mod, 7);
    bn_mod_exp(res, base, exp, mod);
    bn_print("3^10 mod 7", res);

    bn_free(a);
    bn_free(b);
    bn_free(q);
    bn_free(r);
    bn_free(base);
    bn_free(exp);
    bn_free(mod);
    bn_free(res);
    printf("\n");
}

void test_shift()
{
    printf("=== Test: Left & Right Shift ===\n");
    BigNum *a = bn_new();
    BigNum *r = bn_new();

    bn_from_int(a, 0xF);
    bn_print("original a", a);

    bn_lshift(r, a, 4);
    bn_print("a << 4", r);

    bn_rshift(r, r, 2);
    bn_print("result >> 2", r);

    bn_free(a);
    bn_free(r);
    printf("\n");
}

void test_bignum_basic()
{
    printf("=== Testing BigNum Basic Operations ===\n");

    BigNum *a = bn_new();
    BigNum *b = bn_new();
    BigNum *result = bn_new();

    // 测试加法
    // bn_from_int(a, 12345);
    // bn_from_int(b, 67890);
    // bn_add(result, a, b);
    // bn_print("12345 + 67890", result);

    // 测试乘法
    // bn_from_int(a, 275);
    // bn_from_int(b, 1);
    bn_from_hex(a, "ffffaaaaffffaaaa");
    bn_from_hex(b, "123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    bn_mul(result, a, b);
    bn_print("123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef * ffffaaaaffffaaaa", result);
    bn_mul_naive(result, a, b);
    bn_print("123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef * ffffaaaaffffaaaa (naive)", result);
    // 测试大数
    bn_from_hex(a, "FFFFFFFFFFFFFFFF");
    bn_from_hex(b, "2");
    bn_mul(result, a, b);
    bn_print("0xFFFFFFFFFFFFFFFF * 2", result);
    bn_mul_naive(result, a, b);
    bn_print("0xFFFFFFFFFFFFFFFF * 2 (naive)", result);

    bn_free(a);
    bn_free(b);
    bn_free(result);
}

void test_modular_arithmetic()
{
    printf("=== Testing Modular Arithmetic ===\n");

    BigNum *base = bn_new();
    BigNum *exp = bn_new();
    BigNum *mod = bn_new();
    BigNum *result = bn_new();

    // 测试模幂: 3^10 mod 7
    bn_from_int(base, 3);
    bn_from_int(exp, 10);
    bn_from_int(mod, 7);
    bn_mod_exp(result, base, exp, mod);
    bn_print("3^10 mod 7", result);

    // bn_rand(result, 256);
    // bn_print("random result", result);

    // 测试模逆: 3^-1 mod 7
    bn_from_hex(base, "-3");
    bn_from_hex(mod, "5");
    bn_print("base", base);
    bn_print("mod", mod);
    bn_mod_inverse(result, base, mod);
    bn_print("-3^-1 mod 5", result);

    bn_free(base);
    bn_free(exp);
    bn_free(mod);
    bn_free(result);
}

void test_prime_generation()
{
    printf("=== Testing Prime Generation ===\n");

    clock_t start = clock();
    BigNum *prime = prime_generate(256);
    clock_t end = clock();

    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    bn_print("Generated 256-bit prime", prime);
    printf("Time taken: %.3f seconds\n", time_taken);
    BigNum *base = bn_new();
    bn_from_int(base, 25);
    printf("Is prime? %s\n", prime_miller_rabin_test(prime, base) ? "YES" : "NO");

    bn_free(prime);
    bn_free(base);
}

void test_rsa_encrypt_decrypt()
{
    printf("=== Testing RSA Encryption/Decryption ===\n");

    // 生成512位密钥（测试用）
    RSAKey *key = rsa_generate_key(768);
    if (!key)
    {
        printf("Failed to generate key\n");
        return;
    }

    printf("Public key (n):\n");
    bn_print("n", key->n);
    printf("e = %lu\n", bn_to_uint64(key->e));

    // 测试消息
    BigNum *message = bn_new();
    bn_from_hex(message, "48656C6C6F1232132131231243cdcdcdcdcd21321"); // "Hello" in hex
    bn_print("Original message", message);

    // 加密
    BigNum *ciphertext = bn_new();
    clock_t start = clock();
    rsa_encrypt(ciphertext, message, key);
    clock_t end = clock();
    bn_print("Ciphertext", ciphertext);
    printf("Encryption time: %.3f ms\n",
           ((double)(end - start)) / CLOCKS_PER_SEC * 1000);

    // 解密
    BigNum *decrypted = bn_new();
    start = clock();
    rsa_decrypt_crt(decrypted, ciphertext, key);
    end = clock();
    bn_print("Decrypted message", decrypted);
    printf("Decryption time: %.3f ms\n",
           ((double)(end - start)) / CLOCKS_PER_SEC * 1000);

    // 验证
    if (bn_cmp(message, decrypted) == 0)
    {
        printf("Encryption/Decryption PASSED\n");
    }
    else
    {
        printf("Encryption/Decryption FAILED\n");
    }

    bn_free(message);
    bn_free(ciphertext);
    bn_free(decrypted);
    rsa_free_key(key);
}

void test_rsa_sign_verify()
{
    printf("=== Testing RSA Signature ===\n");

    RSAKey *key = rsa_generate_key(2048);
    if (!key)
        return;

    // 模拟消息哈希
    BigNum *hash = bn_new();
    bn_from_hex(hash, "DEADBEEF12345678");
    bn_print("Message", hash);

    // 签名
    BigNum *signature = bn_new();
    rsa_sign(signature, hash, key);
    bn_print("Signature", signature);

    // 验证
    bool valid = rsa_verify(hash, signature, key);
    printf("Signature valid? %s\n", valid ? "YES" : "NO");

    // 测试错误的签名
    BigNum *wrong_hash = bn_new();
    bn_from_hex(wrong_hash, "BAADF00D87654321");
    valid = rsa_verify(wrong_hash, signature, key);
    printf("Wrong hash valid? %s (should be NO)\n", valid ? "YES" : "NO");

    bn_free(hash);
    bn_free(signature);
    bn_free(wrong_hash);
    rsa_free_key(key);
}

void benchmark_key_generation()
{
    printf("=== RSA Key Generation Benchmark ===\n");

    int sizes[] = {256, 512, 768, 1024, 2048, 4096};
    int len = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < len; i++)
    {
        printf("Generating %d-bit key...\n", sizes[i]);

        clock_t start = clock();
        RSAKey *key = rsa_generate_key(sizes[i]);
        clock_t end = clock();

        double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

        if (key)
        {
            printf("%d-bit key generated in %.3f seconds\n",
                   sizes[i], time_taken);
            rsa_free_key(key);
        }
        else
        {
            printf("Failed to generate %d-bit key\n", sizes[i]);
        }
    }
}

void bn_gcd(BigNum *result, const BigNum *a, const BigNum *b)
{
    BigNum *r0 = bn_copy(a);
    BigNum *r1 = bn_copy(b);

    while (!bn_is_zero(r1))
    {
        BigNum *q = bn_new();
        BigNum *r2 = bn_new();
        bn_div(q, r2, r0, r1);
        bn_free(q);

        bn_free(r0);
        r0 = r1;
        r1 = r2;
    }

    if (result->capacity < r0->length)
    {
        bn_resize(result, r0->length);
    }
    memcpy(result->data, r0->data, r0->length * sizeof(uint32_t));
    result->length = r0->length;
    result->sign = r0->sign;

    bn_free(r0);
    bn_free(r1);
}

int main()
{
    printf("RSA Cryptography Test Suite\n");
    printf("============================\n");

    test_init_and_conversion();
    test_add_sub_mul();
    test_div_mod();
    test_shift();
    test_bignum_basic();
    test_modular_arithmetic();
    test_prime_generation();
    test_rsa_encrypt_decrypt();
    test_rsa_sign_verify();
    benchmark_key_generation();
    printf("=== All tests completed ===\n");

    return 0;
}
