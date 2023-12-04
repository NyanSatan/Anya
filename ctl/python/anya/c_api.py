from .exceptions import AnyaBackendError

import os
from ctypes import *

def libanya_load() -> CDLL:
    env = os.environ.get("LIBANYA")
    if env:
        try:
            return CDLL(env)
        except OSError:
            raise AnyaBackendError("couldn't load backend library from LIBANYA env variable")

    try:
        return CDLL("libanya.dylib")
    except OSError:
        raise AnyaBackendError("couldn't load backend library neither from ./libanya.dylib, nor LIBANYA env variable")

backend = libanya_load()

anya_open = backend.anya_open
anya_open.restype = c_int
anya_open.argtypes = [POINTER(c_void_p), c_ulonglong]

anya_close = backend.anya_close
anya_close.restype = c_int
anya_close.argtypes = [POINTER(c_void_p)]

anya_ping_sep = backend.anya_ping_sep
anya_ping_sep.restype = c_int
anya_ping_sep.argtypes = [c_void_p, POINTER(c_bool)]

anya_decrypt = backend.anya_decrypt
anya_decrypt.restype = c_int
anya_decrypt.argtypes = [c_void_p, c_void_p, c_void_p, c_size_t, c_bool]

anya_reboot = backend.anya_reboot
anya_reboot.restype = c_int
anya_reboot.argtypes = [c_void_p]

# uses C's printf() internally, casuses a race condition with Python's print()
"""
anya_print_device = backend.anya_print_device
anya_print_device.restype = None
anya_print_device.argtypes = [c_void_p]
"""

anya_get_cpid = backend.anya_get_cpid
anya_get_cpid.restype = c_ushort
anya_get_cpid.argtypes = [c_void_p]

anya_get_cpfm = backend.anya_get_cpfm
anya_get_cpfm.restype = c_ubyte
anya_get_cpfm.argtypes = [c_void_p]

anya_get_ecid = backend.anya_get_ecid
anya_get_ecid.restype = c_ulonglong
anya_get_ecid.argtypes = [c_void_p]

anya_strerror = backend.anya_strerror
anya_strerror.restype = c_char_p
anya_strerror.argtypes = [c_int]
