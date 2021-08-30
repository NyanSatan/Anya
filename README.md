# Anya

***It's a beta, so be extremely careful when using this software and always validate results it gives you***

**Anya** is a KBAG decryption kit for JTAGable prototypes. Here is the list of platforms currently supported:

* **M9 B0/B1** - Apple S4/S5
* **Cyprus B1** - Apple A12
* **Cebu B1** - Apple A13
* **Sicily A0** - Apple A14 (since beta 3)

*Although SEP support is planned (when applicable), currently Anya only supports AP (application processor) KBAGs*

## Building
### Requirements
* libirecovery
* LLVM toolchain for ARM64

### Building
Build system is a dumpster fire, better just download a ZIP from releases, but still you can try:
```
make
```
...or to build a package (basically, a ZIP):
```
make package
```
List of environmental variables you *might* need to provide:
* `TOOLCHAIN` - path to a directory where *LLVM* toolchain binaries capable of producing ARM64 binaries is located 
* `ARM_CC` - ARM64 C compiler. Suppresses need of `TOOLCHAIN`
* `ARM_LD` - linker capable of using *linker scripts*. Suppresses need of `TOOLCHAIN`
* `ARM_OBJCOPY` - objcopy to cut out raw binary out of an ELF. Suppresses need of `TOOLCHAIN`
* `CC` - C compiler used to compile **anyactl** (client utility)
* `AR` - archiver used to build **libanya** (client library - the utility is based on it)
* `VALID_HANDLER_TARGETS` - list of targets to build USB DFU handler for. Current list of valid targets is **M9/B0_B1**, **Cyprus/B1**, **Cebu/B1** and **Sicily/A0**

In the end you'll get a structure like this in the `build/` folder:
```
build/anya.ax
build/anyactl
build/payloads/anya_handler.M9-B0_B1.bin
build/payloads/anya_handler.Sicily-A0.bin
build/payloads/anya_handler.Cyprus-B1.bin
build/payloads/anya_handler.Cebu-B1.bin
build/python/ (stored 0%)
build/python/requirements.txt
build/python/anyactl
build/python/anya/
build/python/anya/__init__.py
build/python/anya/exceptions.py
build/python/anyafromjson
```

## Usage
### Requirements
* Compiled Anya
* Astris

### Usage
First of all, you need to put your device into Anya mode (basically SecureROM DFU with custom USB control request handler) via Astris:
```
ANYA_PAYLOAD=path/to/desired/payload astris --script path/to/anya.ax
```
***Warning**: this will force reset your device! Better put your device into iBoot recovery or SecureROM DFU mode before doing this!*

Upon successful execution you will get a log like this one:
```
noone@Mac-mini-noone anya % ANYA_PAYLOAD=build/payloads/anya_handler.Cyprus-B1.bin astris --script build/anya.ax
astris v2.7.2

WARNING: This version of tcl is included in macOS for compatibility with legacy software.
In future versions of macOS the tcl runtime will not be available by
default, and may require you to install an additional package.
  

Probe address: KanziSWD-REDACTED
Probe type: kanzi
Probe firmware: 1.24
Probe tckrate: 4420000

  
Listening on port 8000 for ECORE0, ECORE1, ECORE2, ECORE3, PCORE0, PCORE1
Listening on port 8006 for ISP
Listening on port 8007 for GFX
Listening on port 8008 for SIO-DMA
Listening on port 8009 for SIO-AUDIO
Listening on port 8010 for PMP
Listening on port 8011 for SMC
Listening on port 8012 for SEP
Listening on port 8013 for AOP
Listening on port 8014 for AVE-ASC
Listening on port 8015 for AVD
Listening on port 8016 for ANE
Listening on port 8017 for ANS2
Detected Cyprus B1
Loading SOC support script
Identified product D321AP
Fromresetting device...
ECORE0: ASTRIS_ERR_CPUNOTHALTED
ECORE1: ASTRIS_ERR_CPUNOTHALTED
ECORE2: ASTRIS_ERR_CPUNOTHALTED
ECORE3: ASTRIS_ERR_CPUNOTHALTED
PCORE0: ASTRIS_ERR_CPUNOTHALTED
PCORE1: ASTRIS_ERR_CPUNOTHALTED
ISP: ASTRIS_ERR_CPUNOTHALTED
GFX: ASTRIS_ERR_CPUNOTHALTED
SIO-DMA: ASTRIS_ERR_CPUNOTHALTED
SIO-AUDIO: ASTRIS_ERR_CPUNOTHALTED
PMP: ASTRIS_ERR_CPUNOTHALTED
SMC: ASTRIS_ERR_CPUNOTHALTED
SEP: ASTRIS_ERR_CPUNOTHALTED
AOP: ASTRIS_ERR_CPUNOTHALTED
AVE-ASC: ASTRIS_ERR_CPUNOTHALTED
AVD: ASTRIS_ERR_CPUNOTHALTED
ANE: ASTRIS_ERR_CPUNOTHALTED
ANS2: ASTRIS_ERR_CPUNOTHALTED
Dealing with MMU...
bp 0 cleared
bp 0 cleared
Uploading USB handler...
.
338 bytes sent in 0.155 sec, 2181 bytes per second
Forcing DFU...
bp 0 cleared
Patching iBoot flags...
bp 0 cleared
Overriding USB handler ptr...
bp 0 cleared
Unloading SOC support script
noone@Mac-mini-noone anya %
```

Make sure **IBFL** (*iBoot Flags*) value in USB serial number has bit 6 set:
```
CPID:8020 CPRV:11 CPFM:01 SCEP:01 BDID:0E IBFL:6C ECID:REDACTED SRTG:[iBoot-3865.0.0.4.7]
```
This bit is not used by iBoot/SecureROM (except for M1, apparently), so Anya sets it to indicate a device is in Anya mode

Starting from now you can use `anyactl`, usage is quite straight-forward:
```
noone@Mac-mini-noone anya % build/anyactl
usage: build/anyactl ARG[s]

where ARG[s] must be one of the following:
-k KBAG  specifies KBAG to be decrypted
-b NUM  runs benchmark with NUM random KBAGs

you can also use this one with both of the above:
-e ECID  (hexa)decimal ECID to look for

noone@Mac-mini-noone anya %
```

Benchmark output example:
```
noone@Mac-mini-noone anya % build/anyactl -b 10000
found: CPID:8020, CPFM:01, ECID:REDACTED
decrypting...
decrypted 10000 KBAGs in 5.054345 seconds, average - 1978.495728 KBAGs/sec
noone@Mac-mini-noone anya %
```

KBAG decryption output example:
```
noone@Mac-mini-noone anya % build/anyactl -k 9B064DDD9883350334AC486372A1893E38019134A7222D95875AC51CBEEB897CBD3E9003B86C33327A883B796134C81E
found: CPID:8020, CPFM:01, ECID:REDACTED
F9A6BBDE3694D99A9CACCCE890F5BB270A00885FA92754C5451F3688523C0FC7C995D90D33042F4A9967DDC2D757CC0D
noone@Mac-mini-noone anya %
```

***Warning**: since we're using prototype devices here, you obviously need to provide a development KBAG, not production (development one usually comes second in an Image4)!*

## Python
*Available since beta 2*

### Requirements
* pyusb

### Library usage
Placed in `python/anya` folder. Usage is quite straightforward:

```python
from anya import *
from anya.exceptions import *

dev = AnyaDevice(ecid=ECID)	# creating connection class

try:
    dev.connect()	# connecting
except AnyaError as e:
    print("failed to connect: %s" % str(e))
    exit(-1)


try:
    decoded = decode_kbag(KBAG)	# decoding a KBAG string to bytes
except AnyaValueError as e:
    print("failed to parse KBAG: %s" % str(e))
    exit(-1)

try:
    key = dev.decrypt_kbag(decoded)	# decrypting 
except AnyaUSBError as e:
    print("failed to decrypt KBAG: %s" % str(e))
    exit(-1)

print(encode_key(key, to_upper=True)) # encoding key to a string (and printing)
```

### Tools usage
Placed in `python/` folder. There're two of them - **anyactl** (basically the same thing as the C variant of the ctl):

```
noone@Mac-mini-noone anya % build/python/anyactl 
usage: anyactl [-h] [-k KBAG] [-b COUNT] [-e ECID]

Decrypt some KBAG or run a benchmark

optional arguments:
  -h, --help  show this help message and exit
  -k KBAG     decrypt a KBAG
  -b COUNT    run benchmark
  -e ECID     ECID to look for

```

...and **anyafromjson** - this one is used to decrypt KBAGs in batch from a JSON file:

```
noone@Mac-mini-noone anya % build/python/anyafromjson 
usage: build/python/anyafromjson <input> <output> [ecid (hex)]

description:

a dumb utility that takes KBAGs from an input JSON
and decrypts them with Anya. The input JSON must be
a list of dicts where every member must have "kbag"
field. The output JSON will be same, but with "key".
Thus, you can have arbitrary metadata in the dicts
that will be preserved in the output

```

## TODO
* SEP KBAG decryption - very complex task, as it will require a lot of RE on SEPROM and the way to communicate with it from SecureROM
* Improve build system - for the current one is really bad
* Common offset database - so there won't be a need to duplicate some offsets/values in Astris script and USB handlers configs

## Credits
* @axi0mX - for the idea of replacing USB handler (used in **ipwndfu**)
* @pimskeks and other people behind **libimobiledevice** project - for **libirecovery**
* @1nsane_dev - for a lot of tests on Cebu and Sicily
