"""
RSA性能测试脚本
测试加密和解密性能，生成实验报告所需的数据
"""

import time
from py_ui.rsa_wrapper import RSA


def generate_test_message(byte_length):
    """生成指定字节长度的测试消息"""
    # 使用可打印字符生成测试数据
    message = "A" * byte_length
    return message


def get_block_size(n_hex):
    """计算分块大小（字节数）"""
    if n_hex.lower().startswith("0x"):
        n = int(n_hex, 16)
    else:
        n = int(n_hex)
    
    n_bits = n.bit_length()
    block_size = (n_bits - 1) // 8
    return max(1, block_size - 1)


def test_encryption_performance(key_bits, message_bytes, trials=10):
    """
    测试加密性能
    
    Args:
        key_bits: 密钥位数
        message_bytes: 明文字节长度
        trials: 测试轮数
    
    Returns:
        平均耗时(毫秒)
    """
    print(f"\n[加密测试] 密钥位数: {key_bits}, 明文长度: {message_bytes} 字节, 测试轮数: {trials}")
    
    # 生成密钥
    print("  生成密钥中...")
    rsa = RSA(bits=key_bits)
    
    # 生成测试消息
    message = generate_test_message(message_bytes)
    block_size = get_block_size(rsa.n)
    
    print(f"  分块大小: {block_size} 字节")
    print(f"  总分块数: {(message_bytes + block_size - 1) // block_size}")
    
    # 预处理消息
    plaintext_bytes = message.encode('utf-8')
    
    # 预热（第一次运行可能较慢）
    block = plaintext_bytes[:block_size]
    block_hex = block.hex()
    _ = rsa.encrypt(block_hex)
    
    # 正式测试
    times = []
    for trial in range(trials):
        start_time = time.perf_counter()
        
        # 分块加密
        for i in range(0, len(plaintext_bytes), block_size):
            block = plaintext_bytes[i:i + block_size]
            block_hex = block.hex()
            _ = rsa.encrypt(block_hex)
        
        end_time = time.perf_counter()
        elapsed_ms = (end_time - start_time) * 1000
        times.append(elapsed_ms)
        
        print(f"  第 {trial + 1}/{trials} 轮: {elapsed_ms:.2f} ms")
    
    # 计算平均值和标准差
    avg_time = sum(times) / len(times)
    std_dev = (sum((t - avg_time) ** 2 for t in times) / len(times)) ** 0.5
    
    print(f"  ✓ 平均耗时: {avg_time:.2f} ms (标准差: {std_dev:.2f} ms)")
    
    return avg_time, std_dev


def test_decryption_performance(key_bits, message_bytes, trials=10):
    """
    测试解密性能
    
    Args:
        key_bits: 密钥位数
        message_bytes: 明文字节长度
        trials: 测试轮数
    
    Returns:
        平均耗时(毫秒)
    """
    print(f"\n[解密测试] 密钥位数: {key_bits}, 明文长度: {message_bytes} 字节, 测试轮数: {trials}")
    
    # 生成密钥
    print("  生成密钥中...")
    rsa = RSA(bits=key_bits)
    
    # 生成测试消息并加密
    message = generate_test_message(message_bytes)
    block_size = get_block_size(rsa.n)
    plaintext_bytes = message.encode('utf-8')
    
    print(f"  分块大小: {block_size} 字节")
    print(f"  总分块数: {(message_bytes + block_size - 1) // block_size}")
    
    # 预先加密所有块
    print("  预先加密消息...")
    encrypted_blocks = []
    for i in range(0, len(plaintext_bytes), block_size):
        block = plaintext_bytes[i:i + block_size]
        block_hex = block.hex()
        encrypted_block = rsa.encrypt(block_hex)
        encrypted_blocks.append(encrypted_block)
    
    # 预热
    _ = rsa.decrypt(encrypted_blocks[0])
    
    # 正式测试
    times = []
    for trial in range(trials):
        start_time = time.perf_counter()
        
        # 分块解密
        for encrypted_block in encrypted_blocks:
            _ = rsa.decrypt(encrypted_block)
        
        end_time = time.perf_counter()
        elapsed_ms = (end_time - start_time) * 1000
        times.append(elapsed_ms)
        
        print(f"  第 {trial + 1}/{trials} 轮: {elapsed_ms:.2f} ms")
    
    # 计算平均值和标准差
    avg_time = sum(times) / len(times)
    std_dev = (sum((t - avg_time) ** 2 for t in times) / len(times)) ** 0.5
    
    print(f"  ✓ 平均耗时: {avg_time:.2f} ms (标准差: {std_dev:.2f} ms)")
    
    return avg_time, std_dev


def run_encryption_tests():
    """运行所有加密性能测试"""
    print("\n" + "="*70)
    print("加密性能测试")
    print("="*70)
    
    # 测试配置
    test_configs = [
        # (明文字节长度, 密钥位数)
        (100, 1024),
        (100, 2048),
        (100, 4096),
        (200, 1024),
        (400, 1024),
    ]
    
    results = []
    
    for message_bytes, key_bits in test_configs:
        try:
            avg_time, std_dev = test_encryption_performance(
                key_bits=key_bits,
                message_bytes=message_bytes,
                trials=10
            )
            results.append({
                'message_bytes': message_bytes,
                'key_bits': key_bits,
                'avg_time': avg_time,
                'std_dev': std_dev
            })
        except Exception as e:
            print(f"  ✗ 测试失败: {e}")
            results.append({
                'message_bytes': message_bytes,
                'key_bits': key_bits,
                'avg_time': None,
                'std_dev': None
            })
    
    return results


def run_decryption_tests():
    """运行所有解密性能测试"""
    print("\n" + "="*70)
    print("解密性能测试")
    print("="*70)
    
    # 测试配置
    test_configs = [
        # (明文字节长度, 密钥位数)
        (100, 1024),
        (200, 1024),
        (400, 1024),
    ]
    
    results = []
    
    for message_bytes, key_bits in test_configs:
        try:
            avg_time, std_dev = test_decryption_performance(
                key_bits=key_bits,
                message_bytes=message_bytes,
                trials=10
            )
            results.append({
                'message_bytes': message_bytes,
                'key_bits': key_bits,
                'avg_time': avg_time,
                'std_dev': std_dev
            })
        except Exception as e:
            print(f"  ✗ 测试失败: {e}")
            results.append({
                'message_bytes': message_bytes,
                'key_bits': key_bits,
                'avg_time': None,
                'std_dev': None
            })
    
    return results


def print_summary(encryption_results, decryption_results):
    """打印测试总结"""
    print("\n" + "="*70)
    print("测试总结")
    print("="*70)
    
    print("\n加密性能")
    print("-" * 70)
    print(f"{'明文字节长度':<15} {'密钥位数':<12} {'平均耗时(ms)':<15} {'标准差(ms)':<15}")
    print("-" * 70)
    for result in encryption_results:
        if result['avg_time'] is not None:
            print(f"{result['message_bytes']:<15} {result['key_bits']:<12} "
                  f"{result['avg_time']:<15.2f} {result['std_dev']:<15.2f}")
        else:
            print(f"{result['message_bytes']:<15} {result['key_bits']:<12} "
                  f"{'失败':<15} {'N/A':<15}")
    
    print("\n解密性能")
    print("-" * 70)
    print(f"{'明文字节长度':<15} {'密钥位数':<12} {'平均耗时(ms)':<15} {'标准差(ms)':<15}")
    print("-" * 70)
    for result in decryption_results:
        if result['avg_time'] is not None:
            print(f"{result['message_bytes']:<15} {result['key_bits']:<12} "
                  f"{result['avg_time']:<15.2f} {result['std_dev']:<15.2f}")
        else:
            print(f"{result['message_bytes']:<15} {result['key_bits']:<12} "
                  f"{'失败':<15} {'N/A':<15}")


def main():
    """主函数"""
    print("RSA加密解密性能测试")
    print("本测试将评估不同密钥长度和消息长度下的加密解密性能")
    print("测试可能需要较长时间，请耐心等待...")
    
    # 运行加密测试
    encryption_results = run_encryption_tests()
    
    # 运行解密测试
    decryption_results = run_decryption_tests()
    
    # 打印总结
    print_summary(encryption_results, decryption_results)
    
    print("\n测试完成！")


if __name__ == "__main__":
    main()

