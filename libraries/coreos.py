from ctypes import CDLL, c_char_p, c_size_t, c_int, create_string_buffer, byref
import ctypes
import os
import platform

_lib = None

def _try_load():
    global _lib
    names = []
    sys = platform.system()
    if sys == "Linux":
        names = ["libcyon_std.so", "./libcyon_std.so"]
    elif sys == "Darwin":
        names = ["libcyon_std.dylib", "./libcyon_std.dylib"]
    elif sys == "Windows":
        names = ["cyonstd.dll", "./cyonstd.dll", "libcyon_std.dll"]
    else:
        names = ["libcyon_std.so"]
    for n in names:
        try:
            _lib = CDLL(n)
            return True
        except Exception:
            continue
    _lib = None
    return False

_try_load()

# minimal wrappers, fall back to os module when library missing

def getcwd():
    if _lib is None:
        return os.getcwd()
    buf = create_string_buffer(4096)
    rc = _lib.cyon_getcwd(buf, c_size_t(len(buf)))
    if rc == 0:
        return buf.value.decode('utf-8', errors='ignore')
    raise OSError(rc, "cyon_getcwd failed")

def mkdir_p(path):
    if _lib is None:
        os.makedirs(path, exist_ok=True)
        return
    if isinstance(path, str):
        pathb = path.encode('utf-8')
    else:
        pathb = path
    rc = _lib.cyon_mkdir_p(c_char_p(pathb))
    if rc == 0:
        return
    raise OSError(rc, "cyon_mkdir_p failed")

def remove(path):
    if _lib is None:
        os.remove(path)
        return
    if isinstance(path, str):
        pathb = path.encode('utf-8')
    else:
        pathb = path
    rc = _lib.cyon_remove(c_char_p(pathb))
    if rc == 0:
        return
    raise OSError(rc, "cyon_remove failed")

def resolve_path(path):
    if _lib is None:
        return os.path.realpath(path)
    outbuf = create_string_buffer(4096)
    if isinstance(path, str):
        pathb = path.encode('utf-8')
    else:
        pathb = path
    rc = _lib.cyon_resolve_path(c_char_p(pathb), outbuf, c_size_t(len(outbuf)))
    if rc == 0:
        return outbuf.value.decode('utf-8', errors='ignore')
    raise OSError(rc, "cyon_resolve_path failed")

# expose a small convenience: env expand via coreenv if present
def expand_env(s):
    # try to use coreenv if available
    if _lib is None:
        return os.path.expandvars(s)
    # try cyon_env_expand if present
    try:
        _lib.cyon_env_expand.restype = ctypes.c_char_p
        res = _lib.cyon_env_expand(c_char_p(s.encode('utf-8')))
        if not res:
            return s
        # create Python string and free C pointer if necessary
        out = ctypes.cast(res, ctypes.c_char_p).value.decode('utf-8', errors='ignore')
        # try to free if library exposes free (best-effort)
        try:
            _lib.free(res)
        except Exception:
            pass
        return out
    except Exception:
        return os.path.expandvars(s)

# friendly high-level class
class CoreOS:
    def __init__(self):
        pass

    def getcwd(self):
        return getcwd()

    def mkdir_p(self, path):
        return mkdir_p(path)

    def remove(self, path):
        return remove(path)

    def resolve(self, path):
        return resolve_path(path)

    def expand(self, s):
        return expand_env(s)