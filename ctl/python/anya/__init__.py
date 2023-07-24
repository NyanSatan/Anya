from ctypes import *

from .exceptions import * 
from . import c_api

KBAG_SIZE = 0x30

def decode_kbag(kbag: str) -> bytes:
    try:
        assert len(kbag) == KBAG_SIZE * 2
        return bytearray.fromhex(kbag)
    except AssertionError:
        raise AnyaValueError("invalid KBAG length")
    except ValueError:
        raise AnyaValueError("invalid KBAG")

def encode_key(key: bytes, to_upper: bool = False) -> str:
    try:
        assert len(key) == KBAG_SIZE
        key_string = key.hex()
    except (AssertionError, ValueError):
        raise AnyaValueError("invalid key")

    if to_upper:
        return key_string.upper()
    else:
        return key_string

class AnyaDevice:
    def __init__(self, ecid: int = None):
        self.ecid = ecid if ecid else 0
        self.connected = False

        self._connection = c_void_p(0)

    def connect(self):
        status = c_api.anya_open(byref(self._connection), self.ecid)

        if status != 0:
            raise AnyaError("failed to connect - %s" % str(c_api.anya_strerror(status)))
        
        self.connected = True

    def disconnect(self):
        # not really possible to fail here
        c_api.anya_close(byref(self._connection))

        self.connected = False

    def print_device(self):
        print("found: CPID:%04X CPFM:%02X ECID:%016X" % (
                c_api.anya_get_cpid(self._connection),
                c_api.anya_get_cpfm(self._connection),
                c_api.anya_get_ecid(self._connection)
            )
        )

    def decrypt_kbags(self, kbags: list[bytes]) -> list[bytes]:
        all_kbags = b''.join(kbags)
        
        status = c_api.anya_decrypt(self._connection, all_kbags, all_kbags, len(kbags))

        if status != 0:
            raise AnyaError("failed to decrypt - %s" % str(c_api.anya_strerror(status)))
        
        result = list()

        for i in range(len(kbags)):
            result.append(all_kbags[i * KBAG_SIZE : (i + 1) * KBAG_SIZE])

        return result

    def decrypt_kbag(self, kbag: bytes) -> bytes:
        return self.decrypt_kbags([kbag])[0]

    def reboot(self):
        c_api.anya_reboot(self._connection)
        self.disconnect()

    def __del__(self):
        if self.connected:
            self.disconnect()
