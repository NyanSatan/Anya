from .exceptions import *

import usb.core
import usb.util

DFU_VENDOR_ID  = 0x05AC
DFU_PRODUCT_ID = 0x1227 # can also be 0x1226/0x1228 in case of different SDOM

ANYA_IBFL_FLAG   = (1 << 6)
ANYA_USB_TIMEOUT = 100

KBAG_SIZE = 0x30

DFU_DETACH = 0
DFU_DNLOAD = 1
DFU_UPLOAD = 2
DFU_GETSTATUS = 3
DFU_CLR_STATUS = 4
DFU_GETSTATE = 5
DFU_ABORT = 6
ANYA_DECRYPT_KBAG = 7
ANYA_CLEAR_KBAG = 8
ANYA_REBOOT = 9
ANYA_PING_SEP = 10
ANYA_DECRYPT_SEP_KBAG = 11


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
    def __init__(self, vid: int = DFU_VENDOR_ID, pid: int = DFU_PRODUCT_ID, ecid: int = None):
        self.vid = vid
        self.pid = pid
        self.ecid = ecid

    def _match(self, dev):
        IBFL = "IBFL:"
        ECID = "ECID:"

        if dev.idVendor == self.vid and dev.idProduct == self.pid:
            sn = dev.serial_number
            if IBFL in sn:
                ibfl_pos = sn.index(IBFL) + len(IBFL)
                ibfl_val = int(sn[ibfl_pos:ibfl_pos + 2], 16)

                if ibfl_val & ANYA_IBFL_FLAG:
                    if self.ecid is not None:
                        if ECID + "%016X" % self.ecid not in sn:
                            return False

                    return True

        return False

    def connect(self):
        self._device = usb.core.find(custom_match=self._match)
        if not self._device:
            raise AnyaNotFound("no Anya devices found")

        self.clear_kbag()

        print("found: %s" % self._device.serial_number)

    def disconnect(self):
        usb.util.dispose_resources(self._device)

    def ping_sep(self) -> bool:
        try:
            result = self._device.ctrl_transfer(0xA1, ANYA_PING_SEP, 0, 0, 1)
            assert len(result) == 1
        except Exception:
            raise AnyaUSBError("failed to decrypt KBAG")

        return bool(result[0])

    def clear_kbag(self):
        try:
            self._device.ctrl_transfer(0x21, ANYA_CLEAR_KBAG, 0, 0, None)
        except Exception:
            raise AnyaUSBError("failed to clear KBAG")

    def send_kbag(self, kbag: bytes):
        if len(kbag) != KBAG_SIZE:
            raise AnyaValueError("KBAG must be exactly bytes %d in size" % KBAG_SIZE)

        try:
            assert self._device.ctrl_transfer(0x21, DFU_DNLOAD, 0, 0, kbag) == KBAG_SIZE
        except Exception:
            raise AnyaUSBError("failed to send KBAG")

    def get_key(self, sep: bool = False) -> bytes:
        try:
            key = self._device.ctrl_transfer(0xA1, ANYA_DECRYPT_SEP_KBAG if sep else ANYA_DECRYPT_KBAG, 0, 0, KBAG_SIZE)
            assert len(key) == KBAG_SIZE
        except Exception:
            raise AnyaUSBError("failed to decrypt KBAG")

        return bytes(key)

    def decrypt_kbag(self, kbag: bytes, sep: bool = False) -> bytes:
        self.send_kbag(kbag)
        return self.get_key(sep=sep)

    def reboot(self):
        try:
            self._device.ctrl_transfer(0x21, ANYA_REBOOT, 0, 0, None)
        except Exception:
            pass
