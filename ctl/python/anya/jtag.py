# old shitty test code, do not use!
# TODO: rewrite this in C?

from . import KBAG_SIZE
from .exceptions import *

import os
import sys
import time
from ctypes import *
from collections import namedtuple

ASTRIS_FRAMEWORK_PATH = "/Library/Frameworks/Astris.framework/Versions/A/Astris"

PAGE_SIZE = 0x4000

POLL_ATTEMPTS = 20
POLL_PERIOD   = 0.01

ASTRIS_ERR_OK           =  0
ASTRIS_ERR_NOTSUPPORTED = -22

ASTRIS_ARM64_REG_PC        = 32
ASTRIS_ARM64_REG_TPIDR_EL1 = 1343803520

ASTRIS_CPU_STATUS_HWBP = 4

AES_DECRYPT  = 0x11
AES_GID_TYPE = 0x20000200

SEP_AES_BLK      = 0x10
SEP_AES_KEY      = 0x20A
SEP_AES_CLK_REG  = 0x4
SEP_AES_CTL_REG  = 0x8
SEP_AES_STS_REG  = 0xC
SEP_AES_IN_ADDR  = 0x40
SEP_AES_IV_ADDR  = 0x50
SEP_AES_OUT_ADDR = 0x70

SEP_POLL_ATTEMPTS = 20
SEP_POLL_PERIOD   = 0.01

SoCConfig = namedtuple(
    "SoCConfig",
    ["codename", "revision", "main_cpu", "scratch_mem",
     "aes_crypto_cmd_call", "curr_sp_reg", "firebloom",
     "firebloom_kbag_type", "microkernel", "tpidr_elr_off",
     "return_to_el0", "sep_aes_base"]
)

SOC_CONFIGS = [
    SoCConfig(
        codename="Donan",
        revision="A0",
        main_cpu="PCORE0",
        scratch_mem=0x1FC080000,
        aes_crypto_cmd_call=0x100005A68,
        curr_sp_reg=1343766784, # SP_EL0
        firebloom=True,
        firebloom_kbag_type=0x100045210,
        microkernel=True,
        tpidr_elr_off=0xF8,
        return_to_el0=0x1000220DC,
        sep_aes_base=0x3A8B00000
    )
]

class AnyaAstrisDevice:
    def __init__(self, probe: str = None):
        self.probe = probe
        self.connected = False

        if not astris_api.loaded:
            raise AnyaBackendError("call \"astris_api_load()\" before using the Astris backend")

        self._conn = None
        self._soc_name = None
        self._soc_rev = None
        self._dap = None
        self._main_cpu = None
        self._sep_cpu = None

        self._bp = -1

        self._new_sp = -1

    def connect(self):
        self._conn = astris_api.AXconn_create()

        ret = astris_api.AXconn_connect(self._conn, create_string_buffer(self.probe.encode("ascii")), 0, 0)
        if ret != ASTRIS_ERR_OK:
            self.disconnect()
            raise AnyaBackendError("couldn't connect to probe (%s)" % astris_api.strerror(ret))

        ret = astris_api.AXjtag_explore(self._conn, 0)
        if ret < ASTRIS_ERR_OK:
            self.disconnect()
            raise AnyaBackendError("couldn't explore (%s)" % astris_api.strerror(ret))

        self._soc_name = astris_api.AXconn_identify(self._conn).decode("ascii")
        self._soc_rev = astris_api.AXconn_identify_rev(self._conn).decode("ascii")

        try:
            self._soc_config = next(
                filter(
                    lambda x: x.codename == self._soc_name and x.revision == self._soc_rev,
                    SOC_CONFIGS
                )
            )
        except StopIteration:
            name = str(self._soc_name)
            rev = str(self._soc_rev)
            self.disconnect()
            raise AnyaBackendError("no config for %s %s" % (name, rev))

        self._dap = c_void_p(0)
        ret = astris_api.AXchip_find_dap(self._conn, byref(self._dap))
        if ret != ASTRIS_ERR_OK:
            self.disconnect()
            raise AnyaBackendError("couldn't find DAP (%s)" % astris_api.strerror(ret))

        is_dfu = c_bool(0)
        ret = astris_api.AXchip_is_dfu(self._dap, byref(is_dfu))
        if ret != ASTRIS_ERR_OK:
            self.disconnect()
            raise AnyaBackendError("couldn't query DFU status (%s)" % astris_api.strerror(ret))

        if not is_dfu:
            self.disconnect()
            raise AnyaBackendError("device is not in DFU")

        self._main_cpu = astris_api.AXconn_getcpubyname(self._conn, self._soc_config.main_cpu.encode("ascii"))
        if not self._main_cpu:
            self.disconnect()
            raise AnyaBackendError("couldn't get main CPU (%s)" % astris_api.strerror(ret))

        ret = astris_api.astris_cpu_clear_lockacc(self._main_cpu)
        if ret != ASTRIS_ERR_OK and ret != ASTRIS_ERR_NOTSUPPORTED:
            self.disconnect()
            raise AnyaBackendError("couldn't unlock TBM (%s)" % astris_api.strerror(ret))

        ret = astris_api.AXcpu_halt(self._main_cpu)
        if ret != ASTRIS_ERR_OK:
            self.disconnect()
            raise AnyaBackendError("couldn't halt main CPU (%s)" % astris_api.strerror(ret))

        self._bp = astris_api.AXcpu_hwbp_insert(self._main_cpu, self._soc_config.aes_crypto_cmd_call + 4)
        if self._bp < ASTRIS_ERR_OK:
            self.disconnect()
            raise AnyaBackendError("couldn't set a breakpoint to main CPU (%s)" % astris_api.strerror(self._bp))

        if self._soc_config.microkernel:
            current_pc = c_uint64(0)
            ret = astris_api.AXcpu_readreg(self._main_cpu, ASTRIS_ARM64_REG_PC, byref(current_pc))
            if ret != ASTRIS_ERR_OK:
                raise AnyaBackendError("couldn't get current PC (%s)" % astris_api.strerror(ret))

            if current_pc.value != self._soc_config.aes_crypto_cmd_call + 4:
                ret = astris_api.AXcpu_writereg(self._main_cpu, ASTRIS_ARM64_REG_PC, self._soc_config.return_to_el0)
                if ret != ASTRIS_ERR_OK:
                    raise AnyaBackendError("couldn't set new PC on init (%s)" % astris_api.strerror(ret))

                thread_state = c_uint64(0)
                ret = astris_api.AXcpu_readreg(self._main_cpu, ASTRIS_ARM64_REG_TPIDR_EL1, byref(thread_state))
                if ret != ASTRIS_ERR_OK:
                    raise AnyaBackendError("couldn't get TPIDR_EL1 (%s)" % astris_api.strerror(ret))

                self._set_uint64_catchable(self._main_cpu, thread_state.value + self._soc_config.tpidr_elr_off, self._soc_config.aes_crypto_cmd_call + 4)

                if not self._cpu_resume_and_wait_state(self._main_cpu, ASTRIS_CPU_STATUS_HWBP):
                    raise AnyaBackendError("main CPU didn't reach EL0")

        if self._soc_config.firebloom:
            self._new_sp = self._soc_config.scratch_mem + PAGE_SIZE * 4
            ret = astris_api.AXcpu_writereg(self._main_cpu, self._soc_config.curr_sp_reg, self._new_sp)
            if ret != ASTRIS_ERR_OK:
                raise AnyaBackendError("couldn't set new SP (%s)" % astris_api.strerror(ret))

        self._sep_cpu = astris_api.AXconn_getcpubyname(self._conn, "SEP".encode("ascii"))
        if self._sep_cpu:
            ret = astris_api.AXcpu_halt(self._sep_cpu)
            if ret != ASTRIS_ERR_OK:
                # TODO - release the CPU somehow?
                self._sep_cpu = None

        self.connected = True

    def disconnect(self):
        if self._conn:
            if self._main_cpu and self._bp != -1:
                ret = astris_api.AXcpu_hwbp_remove(self._main_cpu, self._bp)
                if ret != ASTRIS_ERR_OK:
                    raise AnyaBackendError("couldn't remove breakpoint (%s)" % astris_api.strerror(ret))

            astris_api.AXconn_destroy(self._conn)
            self._conn = None
            self._soc_name = None
            self._soc_rev = None
            self._dap = None
            self._soc_config = None

        self.connected = False

    def print_device(self):
        print("Detected %s %s" % (self._soc_name, self._soc_rev), file=sys.stderr)

    def ping_sep(self) -> bool:
        return self._sep_cpu is not None

    def decrypt_kbags(self, kbags: list[bytes], sep: bool = False) -> list[bytes]:
        result = list()
        count = len(kbags)

        for idx, kbag in enumerate(kbags):
            result.append(
                self.decrypt_kbag(kbag, sep)
            )

            print("\r%d/%d" % (idx + 1, count), end="", file=sys.stderr)

        print(file=sys.stderr)

        return result

    def decrypt_kbag(self, kbag: bytes, sep: bool = False) -> bytes:
        if not sep:
            return self._decrypt_ap(kbag)
        else:
            return self._decrypt_sep(kbag)

    def _decrypt_ap(self, kbag: bytes) -> bytes:
        kbag = bytes(kbag)

        ret = astris_api.AXcpu_writereg(self._main_cpu, ASTRIS_ARM64_REG_PC, self._soc_config.aes_crypto_cmd_call)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't set new PC (%s)" % astris_api.strerror(ret))

        size = c_uint32(KBAG_SIZE)

        ret = astris_api.AXcpu_writemem_width(self._main_cpu, self._soc_config.scratch_mem, byref(size), kbag, 0, 8)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't upload KBAG (%s)" % astris_api.strerror(ret))

        self._set_reg_catchable(self._main_cpu, 0, AES_DECRYPT)

        if self._soc_config.firebloom:
            self._set_reg_catchable(self._main_cpu, 1, self._soc_config.scratch_mem)
            self._set_reg_catchable(self._main_cpu, 2, self._soc_config.scratch_mem)
            self._set_reg_catchable(self._main_cpu, 3, self._soc_config.scratch_mem + KBAG_SIZE)
            self._set_reg_catchable(self._main_cpu, 4, self._soc_config.firebloom_kbag_type)

            self._set_reg_catchable(self._main_cpu, 5, self._soc_config.scratch_mem)
            self._set_reg_catchable(self._main_cpu, 6, self._soc_config.scratch_mem)
            self._set_reg_catchable(self._main_cpu, 7, self._soc_config.scratch_mem + KBAG_SIZE)
            self._set_uint64_catchable(self._main_cpu, self._new_sp + 0, self._soc_config.firebloom_kbag_type)

            self._set_uint64_catchable(self._main_cpu, self._new_sp + 8, KBAG_SIZE)
            self._set_uint64_catchable(self._main_cpu, self._new_sp + 16, AES_GID_TYPE)

            for i in range(24, 24 + 8 * 8, 8):
                self._set_uint64_catchable(self._main_cpu, self._new_sp + i, 0x0)

        else:
            self._set_reg_catchable(self._main_cpu, 1, self._soc_config.scratch_mem)
            self._set_reg_catchable(self._main_cpu, 2, self._soc_config.scratch_mem)
            self._set_reg_catchable(self._main_cpu, 3, KBAG_SIZE)
            self._set_reg_catchable(self._main_cpu, 4, AES_GID_TYPE)
            self._set_reg_catchable(self._main_cpu, 5, 0x0)
            self._set_reg_catchable(self._main_cpu, 6, 0x0)

        if not self._cpu_resume_and_wait_state(self._main_cpu, ASTRIS_CPU_STATUS_HWBP, self._soc_config.aes_crypto_cmd_call + 4):
            raise AnyaBackendError("main CPU didn't reach breakpoint (timeout)")

        ret_val = c_uint64(0)
        ret = astris_api.AXcpu_readreg(self._main_cpu, 0, byref(ret_val))
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't get X0 (%s)" % astris_api.strerror(ret))

        if ret_val.value != 0:
            raise AnyaBackendError("aes_crypto_cmd() failed (%d)" % ret_val.value)

        size = c_uint32(KBAG_SIZE)
        buf = bytes(KBAG_SIZE)

        ret = astris_api.AXcpu_readmem(self._main_cpu, self._soc_config.scratch_mem, byref(size), buf)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't download key (%s)" % astris_api.strerror(ret))

        return buf

    def _decrypt_sep(self, kbag: bytes) -> bytes:
        kbag = bytes(kbag)

        aes_base = self._soc_config.sep_aes_base

        buf = bytearray(KBAG_SIZE)
        iv = [bytes(4) for i in range(4)]

        self._set_uint32_catchable(self._sep_cpu, aes_base + SEP_AES_CTL_REG, SEP_AES_KEY)

        for block in range(KBAG_SIZE // SEP_AES_BLK):
            for word in range(4):
                idx = word * 4 + block * SEP_AES_BLK
                to_write = int.from_bytes(kbag[idx:idx+4], "little")
                self._set_uint32_catchable(self._sep_cpu, aes_base + SEP_AES_IN_ADDR + word * 4, to_write)

            for iv_word in range(4):
                to_write = int.from_bytes(iv[iv_word], "little")
                self._set_uint32_catchable(self._sep_cpu, aes_base + SEP_AES_IV_ADDR + iv_word * 4, to_write)
                idx = block * SEP_AES_BLK + iv_word * 4
                iv[iv_word] = kbag[idx:idx+4]

            self._set_uint32_catchable(self._sep_cpu, aes_base + SEP_AES_CLK_REG, 0x1)

            aes_done = False

            for _ in range(SEP_POLL_ATTEMPTS):
                sts = self._get_uint32_catchable(self._sep_cpu, aes_base + SEP_AES_STS_REG)
                if sts & 1:
                    aes_done = True
                    break
                time.sleep(SEP_POLL_PERIOD)

            if not aes_done:
                raise AnyaBackendError("SEP AES timeout")

            for out_word in range(4):
                size = c_uint32(4)
                val = bytes(4)
                ret = astris_api.AXcpu_readmem(self._sep_cpu, aes_base + SEP_AES_OUT_ADDR + out_word * 4, byref(size), val)
                if ret != ASTRIS_ERR_OK:
                    raise AnyaBackendError("couldn't get SEP AES output (%s)" % (astris_api.strerror(ret)))

                idx = block * SEP_AES_BLK + out_word * 4

                buf[idx:idx+4] = val

        return bytes(buf)

    def _set_reg_catchable(self, cpu, reg: int, val: int):
        ret = astris_api.AXcpu_writereg(cpu, reg, val)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't set X%d (%s)" % (reg, astris_api.strerror(ret)))

    def _set_uint64_catchable(self, cpu, addr: int, val: int):
        size = c_uint32(8)
        ret = astris_api.AXcpu_writemem_width(cpu, addr, byref(size), val.to_bytes(8, "little"), 0, 8)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't set uint64 0x%x @ 0x%x (%s)" % (addr, val, astris_api.strerror(ret)))

    def _get_uint64_catchable(self, cpu, addr: int) -> int:
        size = c_uint32(8)
        val = bytes(8)
        ret = astris_api.AXcpu_readmem(cpu, addr, byref(size), val)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't get uint32 0x%x @ 0x%x (%s)" % (addr, val, astris_api.strerror(ret)))

        return int.from_bytes(val, "little")

    def _set_uint32_catchable(self, cpu, addr: int, val: int):
        size = c_uint32(4)
        ret = astris_api.AXcpu_writemem_width(cpu, addr, byref(size), val.to_bytes(4, "little"), 0, 4)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't set uint32 0x%x @ 0x%x (%s)" % (addr, val, astris_api.strerror(ret)))

    def _get_uint32_catchable(self, cpu, addr: int) -> int:
        size = c_uint32(4)
        val = bytes(4)
        ret = astris_api.AXcpu_readmem(cpu, addr, byref(size), val)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't get uint32 0x%x @ 0x%x (%s)" % (addr, val, astris_api.strerror(ret)))

        return int.from_bytes(val, "little")

    def _cpu_resume_and_wait_state(self, cpu, state: int, expected_pc: int = None):
        ret = astris_api.AXcpu_resume(cpu)
        if ret != ASTRIS_ERR_OK:
            raise AnyaBackendError("couldn't resume CPU (%s)" % astris_api.strerror(ret))

        reached_bp = False

        for i in range(POLL_ATTEMPTS):
            if astris_api.AXcpu_status(cpu) == state:
                if expected_pc:
                    current_pc = c_uint64(0)
                    ret = astris_api.AXcpu_readreg(self._main_cpu, ASTRIS_ARM64_REG_PC, byref(current_pc))
                    if ret != ASTRIS_ERR_OK:
                        raise AnyaBackendError("couldn't get current PC (%s)" % astris_api.strerror(ret))

                    if expected_pc != current_pc.value:
                        raise AnyaBackendError("caught a wrong breakpoint?!")

                reached_bp = True
                break
            time.sleep(POLL_PERIOD)

        return reached_bp

    def reboot(self):
        self.disconnect()

    def __del__(self):
        if self.connected:
            self.disconnect()


class AstrisAPI:
    def __init__(self):
        self.loaded = False

    def load(self):
        self._backend = CDLL(ASTRIS_FRAMEWORK_PATH)

        self.AXconn_create = self._backend.AXconn_create
        self.AXconn_create.restype = c_void_p
        self.AXconn_create.argtypes = []

        self.AXconn_destroy = self._backend.AXconn_destroy
        self.AXconn_destroy.restype = c_int
        self.AXconn_destroy.argtypes = [c_void_p]

        self.AXconn_connect = self._backend.AXconn_connect
        self.AXconn_connect.restype = c_int
        self.AXconn_connect.argtypes = [c_void_p, c_char_p, c_int, c_int]

        self.AXjtag_explore = self._backend.AXjtag_explore
        self.AXjtag_explore.restype = c_int
        self.AXjtag_explore.argtypes = [c_void_p, c_int]

        self.AXconn_identify = self._backend.AXconn_identify
        self.AXconn_identify.restype = c_char_p
        self.AXconn_identify.argtypes = [c_void_p]

        self.AXconn_identify_rev = self._backend.AXconn_identify_rev
        self.AXconn_identify_rev.restype = c_char_p
        self.AXconn_identify_rev.argtypes = [c_void_p]

        self.AXchip_find_dap = self._backend.AXchip_find_dap
        self.AXchip_find_dap.restype = c_int
        self.AXchip_find_dap.argtypes = [c_void_p, POINTER(c_void_p)]

        self.AXchip_is_dfu = self._backend.AXchip_is_dfu
        self.AXchip_is_dfu.restype = c_int
        self.AXchip_is_dfu.argtypes = [c_void_p, POINTER(c_bool)]

        self.AXconn_getcpubyname = self._backend.AXconn_getcpubyname
        self.AXconn_getcpubyname.restype = c_void_p
        self.AXconn_getcpubyname.argtypes = [c_void_p, c_char_p]

        self.astris_cpu_clear_lockacc = self._backend.astris_cpu_clear_lockacc
        self.astris_cpu_clear_lockacc.restype = c_int
        self.astris_cpu_clear_lockacc.argtypes = [c_void_p]

        self.AXcpu_halt = self._backend.AXcpu_halt
        self.AXcpu_halt.restype = c_int
        self.AXcpu_halt.argtypes = [c_void_p]

        self.AXcpu_resume = self._backend.AXcpu_resume
        self.AXcpu_resume.restype = c_int
        self.AXcpu_resume.argtypes = [c_void_p]

        self.AXcpu_status = self._backend.AXcpu_status
        self.AXcpu_status.restype = c_int
        self.AXcpu_status.argtypes = [c_void_p]

        self.AXcpu_readmem = self._backend.AXcpu_readmem
        self.AXcpu_readmem.restype = c_int
        self.AXcpu_readmem.argtypes = [c_void_p, c_uint64, POINTER(c_uint32), c_void_p]

        self.AXcpu_writemem = self._backend.AXcpu_writemem
        self.AXcpu_writemem.restype = c_int
        self.AXcpu_writemem.argtypes = [c_void_p, c_uint64, POINTER(c_uint32), c_void_p, c_int]

        self.AXcpu_writemem_width = self._backend.AXcpu_writemem_width
        self.AXcpu_writemem_width.restype = c_int
        self.AXcpu_writemem_width.argtypes = [c_void_p, c_uint64, POINTER(c_uint32), c_void_p, c_int, c_int]

        self.AXcpu_readreg = self._backend.AXcpu_readreg
        self.AXcpu_readreg.restype = c_int
        self.AXcpu_readreg.argtypes = [c_void_p, c_uint64, POINTER(c_uint64)]

        self.AXcpu_writereg = self._backend.AXcpu_writereg
        self.AXcpu_writereg.restype = c_int
        self.AXcpu_writereg.argtypes = [c_void_p, c_uint64, c_uint64]

        self.AXcpu_hwbp_insert = self._backend.AXcpu_hwbp_insert
        self.AXcpu_hwbp_insert.restype = c_int
        self.AXcpu_hwbp_insert.argtypes = [c_void_p, c_uint64]

        self.AXcpu_hwbp_remove = self._backend.AXcpu_hwbp_remove
        self.AXcpu_hwbp_remove.restype = c_int
        self.AXcpu_hwbp_remove.argtypes = [c_void_p, c_int]

        self.AXstrerror = self._backend.AXstrerror
        self.AXstrerror.restype = c_char_p
        self.AXstrerror.argtypes = [c_int]

        self.loaded = True

    def strerror(self, err):
        return astris_api.AXstrerror(err).decode("ascii")

astris_api = AstrisAPI()

def astris_api_load():
    astris_api.load()
