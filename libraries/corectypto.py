import ctypes
import os

_lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), "libcyon_std.a"))

# Define return and argument types
_lib.cyon_hash_md5.restype = ctypes.c_char_p
_lib.cyon_hash_sha256.restype = ctypes.c_char_p

def md5(data: str) -> str:
    return _lib.cyon_hash_md5(data.encode()).decode()

def sha256(data: str) -> str:
    return _lib.cyon_hash_sha256(data.encode()).decode()

def random_bytes(size: int) -> bytes:
    buf = (ctypes.c_ubyte * size)()
    _lib.cyon_random_bytes(buf, size)
    return bytes(buf)