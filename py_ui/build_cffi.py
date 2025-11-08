# build_cffi.py
import shutil
import os
from cffi import FFI
import platform

# -----------------------------
# 配置路径
# -----------------------------
project_root = os.path.dirname(os.path.abspath(__file__))
build_dir = os.path.join(project_root, "build")
c_core_include_dir = os.path.join(project_root, "../c_core/include")
c_core_src_dir = os.path.join(project_root, "../c_core/src")

# 清理旧 build
if os.path.exists(build_dir):
    shutil.rmtree(build_dir)
os.makedirs(build_dir, exist_ok=True)

# -----------------------------
# CFFI 初始化
# -----------------------------
ffi = FFI()

# 指定头文件中暴露的函数
ffi.cdef("""    
    void free(void *ptr);

    char *py_generate_key(int bits);
    char *py_rsa_encrypt(const char *n_hex, const char *e_hex, const char *plaintext_hex);
    char *py_rsa_decrypt(const char *n_hex, const char *d_hex,
                         const char *p_hex, const char *q_hex,
                         const char *dp_hex, const char *dq_hex,
                         const char *qinv_hex,
                         const char *ciphertext_hex);
    char *py_rsa_sign(const char *n_hex, const char *d_hex,
                      const char *p_hex, const char *q_hex,
                      const char *dp_hex, const char *dq_hex,
                      const char *qinv_hex,
                      const char *message_hex);
    bool py_rsa_verify(const char *n_hex, const char *e_hex,
                       const char *message_hex, const char *signature_hex);
""")

# 设置生成模块
ffi.set_source(
    "_cffi_crypto",
    '#include "py_api.h"',
    sources=[
        os.path.join(c_core_src_dir, "bignum.c"),
        os.path.join(c_core_src_dir, "bignum_arith.c"),
        os.path.join(c_core_src_dir, "prime.c"),
        os.path.join(c_core_src_dir, "rsa.c"),
        os.path.join(c_core_src_dir, "sha256.c"),
        os.path.join(c_core_src_dir, "py_api.c"),
    ],
    include_dirs=[c_core_include_dir],
    extra_compile_args=["-O3", "-march=native", "-flto"] if platform.system() != "Windows" else ["/O2", "/utf-8"]
)

# -----------------------------
# 编译
# -----------------------------
ffi.compile(tmpdir=build_dir, verbose=True)

print(f"Build finished. The module is in {build_dir}")
