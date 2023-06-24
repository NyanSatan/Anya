from .exceptions import AnyaBackendError

import os
from ctypes import *

def libanya_load():
    env = os.environ.get("LIBANYA")
    if env:
        try:
            return CDLL(env)
        except OSError:
            raise AnyaBackendError("couldn't load backend from LIBANYA env variable")

    try:
        return CDLL("libanya.dylib")
    except OSError:
        raise AnyaBackendError("couldn't load backend library from ./libanya.dylib")

backend = libanya_load()

anya_open = backend.anya_open
anya_open.restype = c_int
anya_open.argtypes = [POINTER(c_void_p), c_ulonglong]

anya_decrypt = backend.anya_decrypt
anya_decrypt.restype = c_int
anya_decrypt.argtypes = [c_void_p, c_void_p, c_void_p, c_size_t]

anya_reboot = backend.anya_reboot
anya_reboot.restype = c_int
anya_reboot.argtypes = [c_void_p]

anya_close = backend.anya_close
anya_close.restype = c_int
anya_close.argtypes = [POINTER(c_void_p)]

anya_print_device = backend.anya_print_device
anya_print_device.restype = None
anya_print_device.argtypes = [c_void_p]

anya_strerror = backend.anya_strerror
anya_strerror.restype = c_char_p
anya_strerror.argtypes = [c_int]
