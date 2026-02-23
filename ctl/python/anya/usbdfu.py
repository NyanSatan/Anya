import os
from ctypes import *
from types import NoneType

from .exceptions import *
from . import KBAG_SIZE
from .debug import *

class AnyaUSBDFUDevice:
    def __init__(self, ecid: int | NoneType = None):
        self.ecid = ecid if ecid else 0
        self.connected = False

        self._connection = c_void_p(0)

        if not c_api.loaded:
            raise AnyaBackendError("call \"c_api_load()\" before using the USB DFU backend")

    def connect(self):
        status = c_api.anya_open(byref(self._connection), self.ecid)

        if status != 0:
            raise AnyaError(c_api.anya_strerror(status).decode())

        self.connected = True

    def disconnect(self):
        # not really possible to fail here
        c_api.anya_close(byref(self._connection))

        self.connected = False

    def get_cpid(self) -> int:
        return c_api.anya_get_cpid(self._connection)

    def get_cpfm(self) -> int:
        return c_api.anya_get_cpfm(self._connection)

    def get_ecid(self) -> int:
        return c_api.anya_get_ecid(self._connection)

    def print_device(self):
        print((BOLD + "CPID:" + RESET + "%04X " + BOLD + "CPFM:" + RESET + "%02X " + BOLD + "ECID:" + RESET + "%016X") % (
                self.get_cpid(),
                self.get_cpfm(),
                self.get_ecid()
            ),
            file=sys.stderr
        )

    def ping_sep(self) -> bool:
        sep_enabled = c_bool(False)

        status = c_api.anya_ping_sep(self._connection, byref(sep_enabled))

        if status != 0:
            raise AnyaError(c_api.anya_strerror(status).decode())

        return sep_enabled.value

    def decrypt_kbags(self, kbags: list[bytes], sep: bool = False) -> list[bytes]:
        all_kbags = b''.join(kbags)

        status = c_api.anya_decrypt(self._connection, all_kbags, all_kbags, len(kbags), sep)

        if status != 0:
            raise AnyaError(c_api.anya_strerror(status).decode())

        result = list()

        for i in range(len(kbags)):
            result.append(all_kbags[i * KBAG_SIZE : (i + 1) * KBAG_SIZE])

        return result

    def decrypt_kbag(self, kbag: bytes, sep: bool = False) -> bytes:
        return self.decrypt_kbags([kbag], sep)[0]

    def reboot(self):
        c_api.anya_reboot(self._connection)
        self.disconnect()

    def __del__(self):
        if self.connected:
            self.disconnect()


LIBANYA_ENV_VAR = "LIBANYA"
LIBANYA_NAME    = "libanya.dylib"

class AnyaCAPI:
    def __init__(self):
        self.loaded = False

    def load(self):
        env = os.environ.get(LIBANYA_ENV_VAR)
        if env:
            try:
                self._backend = CDLL(env)
            except OSError:
                raise AnyaBackendError("couldn't load backend library from LIBANYA env variable")
        else:
            try:
                self._backend = CDLL(LIBANYA_NAME)
            except OSError:
                raise AnyaBackendError("couldn't load backend library neither from ./libanya.dylib, nor via LIBANYA env variable")

        self.anya_open = self._backend.anya_open
        self.anya_open.restype = c_int
        self.anya_open.argtypes = [POINTER(c_void_p), c_ulonglong]

        self.anya_close = self._backend.anya_close
        self.anya_close.restype = c_int
        self.anya_close.argtypes = [POINTER(c_void_p)]

        self.anya_ping_sep = self._backend.anya_ping_sep
        self.anya_ping_sep.restype = c_int
        self.anya_ping_sep.argtypes = [c_void_p, POINTER(c_bool)]

        self.anya_decrypt = self._backend.anya_decrypt
        self.anya_decrypt.restype = c_int
        self.anya_decrypt.argtypes = [c_void_p, c_void_p, c_void_p, c_size_t, c_bool]

        self.anya_reboot = self._backend.anya_reboot
        self.anya_reboot.restype = c_int
        self.anya_reboot.argtypes = [c_void_p]

        self.anya_get_cpid = self._backend.anya_get_cpid
        self.anya_get_cpid.restype = c_ushort
        self.anya_get_cpid.argtypes = [c_void_p]

        self.anya_get_cpfm = self._backend.anya_get_cpfm
        self.anya_get_cpfm.restype = c_ubyte
        self.anya_get_cpfm.argtypes = [c_void_p]

        self.anya_get_ecid = self._backend.anya_get_ecid
        self.anya_get_ecid.restype = c_ulonglong
        self.anya_get_ecid.argtypes = [c_void_p]

        self.anya_strerror = self._backend.anya_strerror
        self.anya_strerror.restype = c_char_p
        self.anya_strerror.argtypes = [c_int]

        self.loaded = True

c_api = AnyaCAPI()

def c_api_load():
    try:
        c_api.load()
    except AnyaBackendError as e:
        error(str(e))
        exit(-1)
