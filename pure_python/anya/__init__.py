from .exceptions import *

import struct

import usb.core
import usb.util

DFU_VENDOR_ID  = 0x05AC
DFU_PRODUCT_ID = 0x1227 # can also be 0x1226/0x1228 in case of different SDOM

ANYA_IBFL_FLAG   = (1 << 6)
ANYA_USB_TIMEOUT = 100

PACKET_MAGIC = 0x414E5941   # 'ANYA'
PACKET_MAX_SIZE = 0x8000
PACKET_FLAG_DECRYPTED = (1 << 0)

KBAG_SIZE = 0x30
KBAG_MAX_COUNT = int((PACKET_MAX_SIZE - 16) / KBAG_SIZE)

DFU_MAX_PACKET_SIZE = 0x800

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

        self.clear_kbags()

        print("found: %s" % self._device.serial_number)

    def disconnect(self):
        usb.util.dispose_resources(self._device)

    def clear_kbags(self):
        try:
            self._device.ctrl_transfer(0x21, ANYA_CLEAR_KBAG, 0, 0, None)
        except Exception:
            raise AnyaUSBError("failed to clear KBAG")

    def send_packet(self, packet: bytes):
        if len(packet) > PACKET_MAX_SIZE:
            raise AnyaValueError("packet must be no more than %d bytes in size" % PACKET_MAX_SIZE)
        
        try:
            index = 0
            while index < len(packet):
                amount = min(len(packet) - index, DFU_MAX_PACKET_SIZE)
                assert self._device.ctrl_transfer(0x21, DFU_DNLOAD, 0, 0, packet[index:index + amount]) == amount
                index += amount
        except Exception:
            raise AnyaUSBError("failed to send packet")

    def get_packet(self, expected_len) -> bytes:
        try:
            result = self._device.ctrl_transfer(0xA1, ANYA_DECRYPT_KBAG, 0, 0, expected_len)
            assert len(result) == expected_len
        except Exception:
            raise AnyaUSBError("failed to decrypt packet")

        return bytes(result)

    def process_packet(self, packet: bytes, target: list):
        (magic, count, flags, _) = struct.unpack("<LLLL", packet[:16])

        if magic != PACKET_MAGIC:
            raise AnyaValueError("invalid magic in out packet")

        if not (flags & PACKET_FLAG_DECRYPTED):
            raise AnyaValueError("out packet is not decrypted")

        for i in range(count):
            target.append(packet[16 + i * KBAG_SIZE : 16 + (i + 1) * KBAG_SIZE])


    def prepare_packet(self, kbags: list[bytes]) -> bytes:
        if len(kbags) > KBAG_MAX_COUNT:
            raise AnyaValueError("too many KBAGs for a single packet, must be no more than %d" % KBAG_MAX_COUNT)

        header = struct.pack("<LLLL", PACKET_MAGIC, len(kbags), 0, 0)

        for kbag in kbags:
            header += kbag
        
        return header


    def decrypt_batch(self, kbags: list[bytes], target: list):
        packet = self.prepare_packet(kbags)

        self.send_packet(packet)

        out_packet = self.get_packet(len(packet))

        self.process_packet(out_packet, target)


    def decrypt_kbags(self, kbags: list[bytes]) -> list[bytes]:
        offset = 0
        count = len(kbags)
        result = list()

        while count != 0:
            current_count = min(count, KBAG_MAX_COUNT)
            current_batch = kbags[offset:offset + current_count]

            self.decrypt_batch(current_batch, result)

            offset += current_count
            count -= current_count

        return result
        

    def reboot(self):
        try:
            self._device.ctrl_transfer(0x21, ANYA_REBOOT, 0, 0, None)
        except Exception:
            pass
