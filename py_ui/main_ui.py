import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import json
from py_ui.rsa_wrapper import RSA

class RSAApp:
    def __init__(self, root):
        self.root = root
        self.root.title("RSA 加密工具")
        self.root.geometry("900x700")
        
        # 当前 RSA 实例
        self.rsa = None
        
        # 创建笔记本(标签页容器)
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill='both', expand=True, padx=10, pady=10)
        
        # 创建各个标签页
        self.create_keygen_tab()
        self.create_encrypt_tab()
        self.create_decrypt_tab()
        self.create_sign_tab()
        self.create_verify_tab()
    
    def create_keygen_tab(self):
        """密钥生成标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text='密钥生成')
        
        # 密钥长度选择
        frame_bits = ttk.Frame(tab)
        frame_bits.pack(pady=20)
        
        ttk.Label(frame_bits, text="密钥长度(bits):").pack(side='left', padx=5)
        self.bits_var = tk.StringVar(value="2048")
        bits_combo = ttk.Combobox(frame_bits, textvariable=self.bits_var, 
                                  values=["256", "512", "768", "1024", "2048", "3072", "4096"], 
                                  width=10, state='readonly')
        bits_combo.pack(side='left', padx=5)
        
        # 生成按钮
        ttk.Button(frame_bits, text="生成密钥对", 
                  command=self.generate_key).pack(side='left', padx=20)
        
        # 密钥显示区域
        frame_display = ttk.LabelFrame(tab, text="生成的密钥", padding=10)
        frame_display.pack(fill='both', expand=True, padx=20, pady=10)
        
        # 公钥
        ttk.Label(frame_display, text="公钥 (n, e):").pack(anchor='w')
        self.public_key_text = scrolledtext.ScrolledText(frame_display, height=6, wrap=tk.WORD)
        self.public_key_text.pack(fill='both', expand=True, pady=5)
        
        # 私钥
        ttk.Label(frame_display, text="私钥 (完整密钥对):").pack(anchor='w', pady=(10, 0))
        self.private_key_text = scrolledtext.ScrolledText(frame_display, height=10, wrap=tk.WORD)
        self.private_key_text.pack(fill='both', expand=True, pady=5)
        
        # 导出按钮
        btn_frame = ttk.Frame(frame_display)
        btn_frame.pack(pady=10)
        ttk.Button(btn_frame, text="复制公钥", 
                  command=lambda: self.copy_to_clipboard(self.public_key_text)).pack(side='left', padx=5)
        ttk.Button(btn_frame, text="复制私钥", 
                  command=lambda: self.copy_to_clipboard(self.private_key_text)).pack(side='left', padx=5)
    
    def create_encrypt_tab(self):
        """加密标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text='加密')
        
        # 公钥输入
        frame_key = ttk.LabelFrame(tab, text="公钥 (JSON格式)", padding=10)
        frame_key.pack(fill='x', padx=20, pady=10)
        
        self.encrypt_key_text = scrolledtext.ScrolledText(frame_key, height=4, wrap=tk.WORD)
        self.encrypt_key_text.pack(fill='x')
        ttk.Button(frame_key, text="从密钥生成页加载", 
                  command=self.load_public_key_to_encrypt).pack(pady=5)
        
        # 明文输入
        frame_plain = ttk.LabelFrame(tab, text="明文 (任意文本)", padding=10)
        frame_plain.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.plaintext_text = scrolledtext.ScrolledText(frame_plain, height=6, wrap=tk.WORD)
        self.plaintext_text.pack(fill='both', expand=True)
        
        # 加密按钮
        ttk.Button(tab, text="加密", command=self.encrypt_message).pack(pady=10)
        
        # 密文输出
        frame_cipher = ttk.LabelFrame(tab, text="密文 (分块十六进制)", padding=10)
        frame_cipher.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.ciphertext_text = scrolledtext.ScrolledText(frame_cipher, height=6, wrap=tk.WORD)
        self.ciphertext_text.pack(fill='both', expand=True)
        
        # 状态标签
        self.encrypt_status = ttk.Label(frame_cipher, text="", foreground="green")
        self.encrypt_status.pack(pady=5)
        
        ttk.Button(frame_cipher, text="复制密文", 
                  command=lambda: self.copy_to_clipboard(self.ciphertext_text)).pack(pady=5)
    
    def create_decrypt_tab(self):
        """解密标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text='解密')
        
        # 私钥输入
        frame_key = ttk.LabelFrame(tab, text="私钥 (完整密钥对 JSON格式)", padding=10)
        frame_key.pack(fill='x', padx=20, pady=10)
        
        self.decrypt_key_text = scrolledtext.ScrolledText(frame_key, height=4, wrap=tk.WORD)
        self.decrypt_key_text.pack(fill='x')
        ttk.Button(frame_key, text="从密钥生成页加载", 
                  command=self.load_private_key_to_decrypt).pack(pady=5)
        
        # 密文输入
        frame_cipher = ttk.LabelFrame(tab, text="密文 (分块十六进制)", padding=10)
        frame_cipher.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.decrypt_ciphertext_text = scrolledtext.ScrolledText(frame_cipher, height=6, wrap=tk.WORD)
        self.decrypt_ciphertext_text.pack(fill='both', expand=True)
        
        # 解密按钮
        ttk.Button(tab, text="解密", command=self.decrypt_message).pack(pady=10)
        
        # 明文输出
        frame_plain = ttk.LabelFrame(tab, text="明文 (文本)", padding=10)
        frame_plain.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.decrypted_text = scrolledtext.ScrolledText(frame_plain, height=6, wrap=tk.WORD)
        self.decrypted_text.pack(fill='both', expand=True)
        
        # 状态标签
        self.decrypt_status = ttk.Label(frame_plain, text="", foreground="green")
        self.decrypt_status.pack(pady=5)
        
        ttk.Button(frame_plain, text="复制明文", 
                  command=lambda: self.copy_to_clipboard(self.decrypted_text)).pack(pady=5)
    
    def create_sign_tab(self):
        """签名标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text='签名')
        
        # 私钥输入
        frame_key = ttk.LabelFrame(tab, text="私钥 (完整密钥对 JSON格式)", padding=10)
        frame_key.pack(fill='x', padx=20, pady=10)
        
        self.sign_key_text = scrolledtext.ScrolledText(frame_key, height=4, wrap=tk.WORD)
        self.sign_key_text.pack(fill='x')
        ttk.Button(frame_key, text="从密钥生成页加载", 
                  command=self.load_private_key_to_sign).pack(pady=5)
        
        # 消息输入
        frame_msg = ttk.LabelFrame(tab, text="消息 (任意文本)", padding=10)
        frame_msg.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.sign_message_text = scrolledtext.ScrolledText(frame_msg, height=6, wrap=tk.WORD)
        self.sign_message_text.pack(fill='both', expand=True)
        
        # 签名按钮
        ttk.Button(tab, text="生成签名", command=self.sign_message).pack(pady=10)
        
        # 签名输出
        frame_sig = ttk.LabelFrame(tab, text="签名 (十六进制)", padding=10)
        frame_sig.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.signature_text = scrolledtext.ScrolledText(frame_sig, height=6, wrap=tk.WORD)
        self.signature_text.pack(fill='both', expand=True)
        
        # 状态标签
        self.sign_status = ttk.Label(frame_sig, text="", foreground="green")
        self.sign_status.pack(pady=5)
        
        ttk.Button(frame_sig, text="复制签名", 
                  command=lambda: self.copy_to_clipboard(self.signature_text)).pack(pady=5)
    
    def create_verify_tab(self):
        """验签标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text='验签')
        
        # 公钥输入
        frame_key = ttk.LabelFrame(tab, text="公钥 (JSON格式)", padding=10)
        frame_key.pack(fill='x', padx=20, pady=10)
        
        self.verify_key_text = scrolledtext.ScrolledText(frame_key, height=4, wrap=tk.WORD)
        self.verify_key_text.pack(fill='x')
        ttk.Button(frame_key, text="从密钥生成页加载", 
                  command=self.load_public_key_to_verify).pack(pady=5)
        
        # 消息输入
        frame_msg = ttk.LabelFrame(tab, text="消息 (任意文本)", padding=10)
        frame_msg.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.verify_message_text = scrolledtext.ScrolledText(frame_msg, height=5, wrap=tk.WORD)
        self.verify_message_text.pack(fill='both', expand=True)
        
        # 签名输入
        frame_sig = ttk.LabelFrame(tab, text="签名 (十六进制)", padding=10)
        frame_sig.pack(fill='both', expand=True, padx=20, pady=10)
        
        self.verify_signature_text = scrolledtext.ScrolledText(frame_sig, height=5, wrap=tk.WORD)
        self.verify_signature_text.pack(fill='both', expand=True)
        
        # 验签按钮
        ttk.Button(tab, text="验证签名", command=self.verify_signature).pack(pady=10)
        
        # 结果显示
        self.verify_result_label = ttk.Label(tab, text="", font=('Arial', 12, 'bold'))
        self.verify_result_label.pack(pady=10)
    
    def generate_key(self):
        """生成密钥对"""
        try:
            bits = int(self.bits_var.get())

            import time
            start = time.perf_counter()
            self.rsa = RSA(bits=bits)
            elapsed = time.perf_counter() - start

            # 显示公钥
            public_key = {"n": self.rsa.n, "e": self.rsa.e}
            self.public_key_text.delete(1.0, tk.END)
            self.public_key_text.insert(1.0, json.dumps(public_key, indent=2))

            # 显示私钥(完整密钥对)
            self.private_key_text.delete(1.0, tk.END)
            self.private_key_text.insert(1.0, json.dumps(self.rsa.key, indent=2))

            # 格式化耗时
            duration = f"{elapsed*1000:.0f} ms" if elapsed < 1 else f"{elapsed:.2f} 秒"
            messagebox.showinfo("成功", f"成功生成 {bits} 位 RSA 密钥对！\n耗时：{duration}")
        except Exception as e:
            messagebox.showerror("错误", f"生成密钥失败：{str(e)}")
    
    def load_public_key_to_encrypt(self):
        """加载公钥到加密页"""
        try:
            public_key = {"n": self.rsa.n, "e": self.rsa.e}
            self.encrypt_key_text.delete(1.0, tk.END)
            self.encrypt_key_text.insert(1.0, json.dumps(public_key, indent=2))
        except:
            messagebox.showerror("错误", "请先生成密钥对！")
    
    def load_private_key_to_decrypt(self):
        """加载私钥到解密页"""
        try:
            self.decrypt_key_text.delete(1.0, tk.END)
            self.decrypt_key_text.insert(1.0, json.dumps(self.rsa.key, indent=2))
        except:
            messagebox.showerror("错误", "请先生成密钥对！")
    
    def load_private_key_to_sign(self):
        """加载私钥到签名页"""
        try:
            self.sign_key_text.delete(1.0, tk.END)
            self.sign_key_text.insert(1.0, json.dumps(self.rsa.key, indent=2))
        except:
            messagebox.showerror("错误", "请先生成密钥对！")
    
    def load_public_key_to_verify(self):
        """加载公钥到验签页"""
        try:
            public_key = {"n": self.rsa.n, "e": self.rsa.e}
            self.verify_key_text.delete(1.0, tk.END)
            self.verify_key_text.insert(1.0, json.dumps(public_key, indent=2))
        except:
            messagebox.showerror("错误", "请先生成密钥对！")
    
    def get_block_size(self, n):
        """计算分块大小（字节数）"""
        # 如果n是字符串，先转换为整数
        if isinstance(n, str):
            if n.lower().startswith("0x"):
                n = int(n, 16)
            else:
                n = int(n)
        
        # n的位数
        n_bits = n.bit_length()
        # 转换为字节数，预留一些空间避免超过n
        # 使用 (n_bits - 1) // 8 确保加密后的数字小于n
        block_size = (n_bits - 1) // 8
        # 再减少一些字节以确保安全（考虑到填充等因素）
        return max(1, block_size - 1)
    
    def encrypt_message(self):
        """分块加密消息"""
        # 参数校验
        key_text = self.encrypt_key_text.get(1.0, tk.END).strip()
        plaintext = self.plaintext_text.get(1.0, tk.END).strip()
        if not key_text:
            self.encrypt_status.config(text="✗ 公钥不能为空", foreground="red")
            return
        if not plaintext:
            self.encrypt_status.config(text="✗ 明文不能为空", foreground="red")
            return
        try:
            # 解析公钥
            key_json = json.loads(self.encrypt_key_text.get(1.0, tk.END))
            rsa = RSA(key_json=key_json)
            
            # 获取明文字节
            plaintext_bytes = plaintext.encode('utf-8')
            
            # 计算分块大小
            block_size = self.get_block_size(rsa.n)
            
            # 分块加密
            encrypted_blocks = []
            total_blocks = (len(plaintext_bytes) + block_size - 1) // block_size
            
            for i in range(0, len(plaintext_bytes), block_size):
                block = plaintext_bytes[i:i + block_size]
                block_hex = block.hex()
                
                # 加密该块
                encrypted_block = rsa.encrypt(block_hex)
                encrypted_blocks.append(encrypted_block)
            
            # 将所有加密块用分隔符连接
            ciphertext = '|'.join(encrypted_blocks)
            
            # 显示密文
            self.ciphertext_text.delete(1.0, tk.END)
            self.ciphertext_text.insert(1.0, ciphertext)
            
            # 显示状态
            self.encrypt_status.config(
                text=f"✓ 加密成功（共 {total_blocks} 块，每块 {block_size} 字节）", 
                foreground="green"
            )
        except Exception as e:
            self.encrypt_status.config(text=f"✗ 加密失败: {str(e)}", foreground="red")
            self.ciphertext_text.delete(1.0, tk.END)
    
    def decrypt_message(self):
        """分块解密消息"""
        # 参数校验
        key_text = self.decrypt_key_text.get(1.0, tk.END).strip()
        ciphertext = self.decrypt_ciphertext_text.get(1.0, tk.END).strip()
        if not key_text:
            self.decrypt_status.config(text="✗ 私钥不能为空", foreground="red")
            return
        if not ciphertext:
            self.decrypt_status.config(text="✗ 密文不能为空", foreground="red")
            return
        try:
            # 解析私钥
            key_json = json.loads(self.decrypt_key_text.get(1.0, tk.END))
            rsa = RSA(key_json=key_json)
            
            # 获取密文并分割成块
            ciphertext = self.decrypt_ciphertext_text.get(1.0, tk.END).strip()
            encrypted_blocks = ciphertext.split('|')
            
            # 解密所有块
            decrypted_bytes = b''
            for block in encrypted_blocks:
                # 解密该块
                plaintext_hex = str(rsa.decrypt(block))
                if plaintext_hex.lower().startswith("0x"):
                    plaintext_hex = plaintext_hex[2:]
                
                # 确保十六进制字符串长度为偶数
                if len(plaintext_hex) % 2 != 0:
                    plaintext_hex = '0' + plaintext_hex
                
                # 转换为字节
                block_bytes = bytes.fromhex(plaintext_hex)
                decrypted_bytes += block_bytes
            
            # 将字节转换为文本
            plaintext = decrypted_bytes.decode('utf-8')
            
            # 显示明文
            self.decrypted_text.delete(1.0, tk.END)
            self.decrypted_text.insert(1.0, plaintext)
            
            # 显示状态
            self.decrypt_status.config(
                text=f"✓ 解密成功（共 {len(encrypted_blocks)} 块）", 
                foreground="green"
            )
        except Exception as e:
            self.decrypt_status.config(text=f"✗ 解密失败: {str(e)}", foreground="red")
            self.decrypted_text.delete(1.0, tk.END)
    
    def sign_message(self):
        """签名消息"""
        key_text = self.sign_key_text.get(1.0, tk.END).strip()
        message = self.sign_message_text.get(1.0, tk.END).strip()
        if not key_text:
            self.sign_status.config(text="✗ 私钥不能为空", foreground="red")
            return
        if not message:
            self.sign_status.config(text="✗ 消息不能为空", foreground="red")
            return
        try:
            # 解析私钥
            key_json = json.loads(self.sign_key_text.get(1.0, tk.END))
            rsa = RSA(key_json=key_json)
            
            # 获取消息并转换为十六进制
            message = self.sign_message_text.get(1.0, tk.END).strip()
            message_bytes = message.encode('utf-8')
            message_hex = message_bytes.hex()
            
            # 签名
            signature = rsa.sign(message_hex)
            
            # 显示签名
            self.signature_text.delete(1.0, tk.END)
            self.signature_text.insert(1.0, signature)
            
            # 显示状态
            self.sign_status.config(text="✓ 签名成功", foreground="green")
        except Exception as e:
            self.sign_status.config(text=f"✗ 签名失败: {str(e)}", foreground="red")
            self.signature_text.delete(1.0, tk.END)
    
    def verify_signature(self):
        """验证签名"""
        key_text = self.verify_key_text.get(1.0, tk.END).strip()
        message = self.verify_message_text.get(1.0, tk.END).strip()
        signature = self.verify_signature_text.get(1.0, tk.END).strip()

        # 参数校验
        if not key_text:
            self.verify_result_label.config(text="✗ 公钥不能为空", foreground="red")
            return
        if not message:
            self.verify_result_label.config(text="✗ 消息不能为空", foreground="red")
            return
        if not signature:
            self.verify_result_label.config(text="✗ 签名不能为空", foreground="red")
            return
        try:
            # 解析公钥
            key_json = json.loads(self.verify_key_text.get(1.0, tk.END))
            rsa = RSA(key_json=key_json)
            
            # 获取消息并转换为十六进制
            message = self.verify_message_text.get(1.0, tk.END).strip()
            message_bytes = message.encode('utf-8')
            message_hex = message_bytes.hex()
            
            # 获取签名
            signature = self.verify_signature_text.get(1.0, tk.END).strip()
            
            # 验证
            result = rsa.verify(message_hex, signature)
            
            # 显示结果
            if result:
                self.verify_result_label.config(text="✓ 签名验证成功", foreground="green")
            else:
                self.verify_result_label.config(text="✗ 签名验证失败", foreground="red")
        except Exception as e:
            self.verify_result_label.config(text=f"✗ 验证出错: {str(e)}", foreground="red")
    
    def copy_to_clipboard(self, text_widget):
        """复制文本到剪贴板"""
        content = text_widget.get(1.0, tk.END).strip()
        self.root.clipboard_clear()
        self.root.clipboard_append(content)
        messagebox.showinfo("成功", "已复制到剪贴板！")

def main():
    root = tk.Tk()
    app = RSAApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()