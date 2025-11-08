# py_ui/rsa_wrapper.py
from .build._cffi_crypto import lib, ffi
import json

class RSA:
    def __init__(self, key_json=None, bits=2048):
        """
        如果 key_json 为 None，则生成新密钥
        """
        if key_json is None:
            c_str = lib.py_generate_key(bits)
            py_str = ffi.string(c_str).decode()
            lib.free(c_str)
            full_key = json.loads(py_str)
            self.key = {
                'n': full_key['public_key']['n'],
                'e': full_key['public_key']['e'],
                'd': full_key['private_key']['d'],
                'p': full_key['private_key']['p'],
                'q': full_key['private_key']['q'],
                'dp': full_key['private_key']['dp'],
                'dq': full_key['private_key']['dq'],
                'qinv': full_key['private_key']['qinv']
            }
        else:
            self.key = key_json

    @property
    def n(self): return self.key['n']
    @property
    def e(self): return self.key['e']
    @property
    def d(self): return self.key.get('d')
    @property
    def p(self): return self.key.get('p')
    @property
    def q(self): return self.key.get('q')
    @property
    def dp(self): return self.key.get('dp')
    @property
    def dq(self): return self.key.get('dq')
    @property
    def qinv(self): return self.key.get('qinv')

    def encrypt(self, plaintext_hex):
        c_str = lib.py_rsa_encrypt(self.n.encode(), self.e.encode(), plaintext_hex.encode())
        py_str = ffi.string(c_str).decode()
        lib.free(c_str)
        return py_str

    def decrypt(self, ciphertext_hex):
        c_str = lib.py_rsa_decrypt(
            self.n.encode(), self.d.encode(),
            self.p.encode(), self.q.encode(),
            self.dp.encode(), self.dq.encode(), self.qinv.encode(),
            ciphertext_hex.encode()
        )
        py_str = ffi.string(c_str).decode()
        lib.free(c_str)
        return py_str

    def sign(self, message_hex):
        c_str = lib.py_rsa_sign(
            self.n.encode(), self.d.encode(),
            self.p.encode(), self.q.encode(),
            self.dp.encode(), self.dq.encode(), self.qinv.encode(),
            message_hex.encode()
        )
        py_str = ffi.string(c_str).decode()
        lib.free(c_str)
        return py_str

    def verify(self, message_hex, signature_hex):
        return lib.py_rsa_verify(
            self.n.encode(), self.e.encode(),
            message_hex.encode(), signature_hex.encode()
        )
