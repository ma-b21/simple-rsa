# py_ui/test_import.py
from .build._cffi_crypto import lib, ffi
import json
# 假设 c_str 是返回的 char*，例如：
c_str = lib.py_generate_key(2048)

# 转成 Python 字符串
py_str = ffi.string(c_str).decode('utf-8')

a = json.loads(py_str)
print(json.dumps(a, indent=2))