
/**
 * test_prime.c - 素数生成库测试程序
 */

#include "prime.h"
#include <stdio.h>
#include <time.h>

void test_small_primes(void) {
    printf("========== 测试小素数 ==========\n");
    
    BigNum *n = bn_new();
    
    // 测试已知素数
    int test_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
    for (long unsigned int i = 0; i < sizeof(test_primes)/sizeof(int); i++) {
        bn_from_int(n, test_primes[i]);
        bool is_prime = prime_is_prime(n);
        printf("%d: %s\n", test_primes[i], is_prime ? "素数✓" : "合数✗");
    }
    
    // 测试已知合数
    int test_composites[] = {4, 6, 8, 9, 10, 12, 14, 15, 16, 18, 20};
    for (long unsigned int i = 0; i < sizeof(test_composites)/sizeof(int); i++) {
        bn_from_int(n, test_composites[i]);
        bool is_prime = prime_is_prime(n);
        printf("%d: %s\n", test_composites[i], is_prime ? "素数✗" : "合数✓");
    }
    
    bn_free(n);
    printf("\n");
}

void test_generate_prime(int bits) {
    printf("========== 生成 %d 位素数 ==========\n", bits);
    
    clock_t start = clock();
    
    BigNum *p = bn_new();
    int candidates = prime_generate_ex(p, bits);
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("生成的素数: ");
    bn_print("p", p);
    
    printf("位数: %d\n", bn_bit_length(p));
    printf("测试的候选数: %d\n", candidates);
    printf("耗时: %.3f 秒\n", time_spent);
    
    // 验证是否真的是素数
    printf("验证素性: %s\n", prime_is_prime(p) ? "通过✓" : "失败✗");
    
    bn_free(p);
    printf("\n");
}

void benchmark_prime_generation(void) {
    printf("========== 素数生成性能基准测试 ==========\n");
    
    int bit_sizes[] = {64, 128, 256, 512, 1024, 2048};
    int num_tests = sizeof(bit_sizes) / sizeof(int);
    
    printf("%-10s %-15s %-15s %-15s\n", "位数", "平均耗时(秒)", "平均候选数", "测试次数");
    printf("--------------------------------------------------------\n");
    
    for (int i = 0; i < num_tests; i++) {
        int bits = bit_sizes[i];
        int trials = (bits <= 512) ? 10 : 8;  // 大数测试次数少一些
        
        double total_time = 0.0;
        int total_candidates = 0;
        
        for (int j = 0; j < trials; j++) {
            BigNum *p = bn_new();
            
            clock_t start = clock();
            int candidates = prime_generate_ex(p, bits);
            clock_t end = clock();
            
            total_time += (double)(end - start) / CLOCKS_PER_SEC;
            total_candidates += candidates;
            
            bn_free(p);
        }
        
        printf("%-10d %-15.3f %-15.1f %-15d\n", 
               bits, 
               total_time / trials, 
               (double)total_candidates / trials,
               trials);
    }
    
    printf("\n");
}

int main(void) {
    printf("====================================\n");
    printf("   大素数生成库测试程序\n");
    printf("====================================\n\n");
    
    test_small_primes();
    
    test_generate_prime(64);
    test_generate_prime(128);
    test_generate_prime(256);
    
    benchmark_prime_generation();
    
    printf("====================================\n");
    printf("   测试完成！\n");
    printf("====================================\n");
    
    return 0;
}
