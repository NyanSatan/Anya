from .exceptions import * 

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

from .usbdfu import c_api_load
from .usbdfu import AnyaUSBDFUDevice as AnyaDevice
from .tag import print_tag
