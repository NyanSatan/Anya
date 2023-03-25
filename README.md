![](repo/anya_logo_with_girl.png)

***It's a beta, so be extremely careful when using this software and always validate results it gives you***

**Anya** is a KBAG decryption kit for JTAGable prototypes. Here is the list of platforms currently supported:

### AP

* **M9 B0/B1** - Apple S4/S5
* **Cyprus B0/B1** - Apple A12 (B0 since beta 4)
* **Cebu B0/B1** - Apple A13 (B0 since beta 6)
* **Sicily A0** - Apple A14 (since beta 3)
* **Tonga A0** - Apple M1 (since beta 6)
* **Turks B0** - Apple S6/S7 (since beta 6)

### SEP

* **M9 B0/B1** - Apple S4/S5
* **Cyprus B1** - Apple A12

***SEP** support is available since beta 5 and deeply experimental*

## Building
### Requirements
* libirecovery
* LLVM toolchain for ARM64
* Python 3

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
* `PYTHON` - Python 3 interpreter used by some build scripts
* `VALID_HANDLER_TARGETS` - list of targets to build USB DFU handler for. Current list of valid targets is **M9/B0_B1**, **Cyprus/B0**, **Cyprus/B1**, **Cebu/B0_B1**, **Sicily/A0**, **Tonga/A0** and **Turks/B0**
* `VALID_SEP_HANDLER_TARGETS` - list of targets to build SEP mailbox handler for. Current list of valid targets is **M9/B0_B1** and **Cyprus/B1**

In the end you'll get a structure like this in the `build/` folder:
```
anya.ax
anyactl
payloads/
payloads/anya_handler.M9-B0_B1.bin
payloads/anya_handler.Sicily-A0.bin
payloads/anya_handler.Cyprus-B0.bin
payloads/anya_handler.Cyprus-B1.bin
payloads/anya_handler.Cebu-B0_B1.bin
build/payloads/anya_handler.Tonga-A0.bin
build/payloads/anya_handler.Turks-B0.bin
python/
python/requirements.txt
python/anyactl
python/anya/
python/anya/__init__.py
python/anya/exceptions.py
python/anyafromjson
sep_payloads/
sep_payloads/anya_sep_handler.M9-B0_B1.bin
sep_payloads/anya_sep_handler.Cyprus-B1.bin
```

## Usage
### Requirements
* Compiled Anya
* Astris (Yukon tools or later)

### Usage
First of all, you need to put your device into Anya mode (basically SecureROM DFU with custom USB control request handler) via Astris:
```
ANYA_PAYLOAD=path/to/desired/payload astris --script path/to/anya.ax
```
***Warning**: this will force reset your device! Better put your device into iBoot recovery or SecureROM DFU mode before doing this! On devices with a display the DFU mode is strictly recommended, otherwise you'll see weird glitches on it or even potentially damage it!*

Upon successful execution you will get a log like this one:
```
noone@Mac-mini-noone Anya % ANYA_PAYLOAD=build/payloads/anya_handler.Cyprus-B1.bin astris --script build/anya.ax
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
noone@Mac-mini-noone Anya %
```

Make sure **IBFL** (*iBoot Flags*) value in USB serial number has bit 6 set:
```
CPID:8020 CPRV:11 CPFM:01 SCEP:01 BDID:0E IBFL:6C ECID:REDACTED SRTG:[iBoot-3865.0.0.4.7]
```
This bit is not used by iBoot/SecureROM (except for M1, apparently), so Anya sets it to indicate a device is in Anya mode

Starting from now you can use `anyactl`, usage is quite straight-forward:
```
noone@Mac-mini-noone Anya % build/anyactl 
usage: build/anyactl ARG[s]

where ARG[s] must be one of the following:
    -k KBAG specifies KBAG to be decrypted
    -b NUM  runs benchmark with NUM random KBAGs
    -s  uses SEP GID (if possible)

you can also use this one with both of the above:
    -e ECID (hexa)decimal ECID to look for

noone@Mac-mini-noone Anya %
```

Benchmark output example:
```
noone@Mac-mini-noone Anya % build/anyactl -b 10000
found: CPID:8020, CPFM:01, ECID:REDACTED
decrypting...
decrypted 10000 KBAGs in 5.054345 seconds, average - 1978.495728 KBAGs/sec
noone@Mac-mini-noone anya %
```

KBAG decryption output example:
```
noone@Mac-mini-noone Anya % build/anyactl -k 9B064DDD9883350334AC486372A1893E38019134A7222D95875AC51CBEEB897CBD3E9003B86C33327A883B796134C81E
found: CPID:8020, CPFM:01, ECID:REDACTED
F9A6BBDE3694D99A9CACCCE890F5BB270A00885FA92754C5451F3688523C0FC7C995D90D33042F4A9967DDC2D757CC0D
noone@Mac-mini-noone Anya %
```

***Warning**: since we're using prototype devices here, you obviously need to provide a development KBAG, not production (development one usually comes second in an Image4)!*

### SEP notes
*Available since beta 5*

If your device is *insecure* and there's SEP support for a desired platform in Anya:
```
ANYA_PAYLOAD=path/to/desired/payload ANYA_SEP_PAYLOAD=path/to/desired/sep_payload astris --script path/to/anya.ax
```
Note `ANYA_SEP_PAYLOAD` is required now

Upon successful execution you will get a log very simialar to the one without SEP support:
```
noone@Mac-mini-noone Anya % ANYA_PAYLOAD=build/payloads/anya_handler.M9-B0_B1.bin ANYA_SEP_PAYLOAD=build/sep_payloads/anya_sep_handler.M9-B0_B1.bin astris nova --script build/anya.ax
astris v2.7.2

WARNING: This version of tcl is included in macOS for compatibility with legacy software. 
In future versions of macOS the tcl runtime will not be available by 
default, and may require you to install an additional package.


Probe address: Nova-REDACTED
Probe type: kanzi
Probe firmware: 1.26
Probe tckrate: 4420000

Listening on port 8000 for CPU0, CPU1
Listening on port 8002 for SIO_DMA
Listening on port 8003 for SMC
Listening on port 8004 for SEP
Listening on port 8005 for AOP
Listening on port 8006 for RTP
Listening on port 8007 for ANS2
Listening on port 8008 for MSP
Listening on port 8009 for SCE
Listening on port 8010 for GFX
Listening on port 8011 for ISP
Listening on port 8012 for AVD
Listening on port 8013 for ANE
Listening on port 8014 for SDIO
Listening on port 8015 for BTMAC
Listening on port 8016 for WLMAC
Detected M9 B0
Loading SOC support script
Identified product T1146sAP
Fromresetting device...
CPU0: ASTRIS_ERR_CPUNOTHALTED
CPU1: ASTRIS_ERR_CPUNOTHALTED
SIO_DMA: ASTRIS_ERR_CPUNOTHALTED
SMC: ASTRIS_ERR_CPUNOTHALTED
SEP: ASTRIS_ERR_CPUNOTHALTED
AOP: ASTRIS_ERR_CPUNOTHALTED
RTP: ASTRIS_ERR_CPUNOTHALTED
ANS2: ASTRIS_ERR_CPUNOTHALTED
MSP: ASTRIS_ERR_CPUNOTHALTED
SCE: ASTRIS_ERR_CPUNOTHALTED
GFX: ASTRIS_ERR_CPUNOTHALTED
ISP: ASTRIS_ERR_CPUNOTHALTED
AVD: ASTRIS_ERR_CPUNOTHALTED
ANE: ASTRIS_ERR_CPUNOTHALTED
SDIO: ASTRIS_ERR_CPUNOTHALTED
BTMAC: ASTRIS_ERR_CPUNOTHALTED
WLMAC: ASTRIS_ERR_CPUNOTHALTED
Dealing with MMU...
bp 0 cleared
bp 0 cleared
Uploading USB handler...
.
998 bytes sent in 0.363 sec, 2749 bytes per second
Forcing DFU...
bp 0 cleared
Patching iBoot flags...
bp 0 cleared
Overriding USB handler ptr...
bp 0 cleared
Setting up SEP...
SEP: ASTRIS_ERR_OK
Uploading SEP handler...
.
624 bytes sent in 0.222 sec, 2811 bytes per second
Dealing with SEP MMU...
bp 0 cleared
bp 0 cleared
bp 0 cleared
Replacing SEP LR with our handler...
Unloading SOC support script
noone@Mac-mini-noone Anya %
```

To decrypt with SEP GID, add `-s` flag to `anyactl` (both C and Python version):
```
noone@Mac-mini-noone Anya % build/anyactl -k 533AB83177837FAD831105140400AEB75C86C59F742292B12FA5A71749B1CD6F5948BBFE39129B9472DC6DA22C64B3F0 -s
found: CPID:8006, CPFM:00, ECID:REDACTED
CB9076B542287EB5F20CD40DD8DC1B471FC06050E221491C61AD3BE717EFD95A2F4589A3C3BF77ED7A5CE9A8E79C25A9
noone@Mac-mini-noone Anya %
```


## Python
*Available since beta 2*

### Requirements
* Python 3
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
    sep_available = dev.ping_sep() # pinging SEP
except AnyaUSBError as e:
    print("failed to ping SEP: %s" % str(e))

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

dev.disconnect() # disconnecting
```

### Tools usage
Placed in `python/` folder. There're two of them - **anyactl** (basically the same thing as the C variant of the ctl):

```
noone@Mac-mini-noone Anya % build/python/anyactl 
usage: anyactl [-h] [-k KBAG] [-b COUNT] [-s] [-e ECID]

Decrypt some KBAG or run a benchmark

optional arguments:
  -h, --help  show this help message and exit
  -k KBAG     decrypt a KBAG
  -b COUNT    run benchmark
  -s          use SEP GID (if possible)
  -e ECID     ECID to look for
noone@Mac-mini-noone Anya % 
```

...and **anyafromjson** - this one is used to decrypt KBAGs in batch from a JSON file:

```
noone@Mac-mini-noone Anya % python/anyafromjson --help
usage: anyafromjson [-h] [-e ECID] [-s] [-d] in_path out_path

a dumb utility that takes KBAGs from an input JSON
and decrypts them with Anya. The input JSON must be
a list of dicts where every member must have "kbag"
field. The output JSON will be same, but with "key"
Thus, you can have arbitrary metadata in the dicts
that will be preserved in the output

to decrypt all KBAGs with SEP GID use "-s" flag, if
you wanna supply mixed file (both AP and SEP) KBAGs,
then try "-d" flag - in this case it will try to 
detect SEP KBAGs by looking at "filename" field -
if it starts with "SEP" (case does not matter),
then SEP GID is gonna be used. If none of those is
provided, everything will be decrypted with AP GID

positional arguments:
  in_path     input file
  out_path    output file

optional arguments:
  -h, --help  show this help message and exit
  -e ECID     (hexadecimal) ECID to look for
  -s          use SEP GID (if possible)
  -d          try to detect SEP KBAGs
noone@Mac-mini-noone Anya %
```

## TODO
* Improve build system - for the current one is really bad
* Common offset database - so there won't be a need to duplicate some offsets/values in the Astris script and USB handlers configs
* A13+ SEP support - gonna be a tough task, because of Secure Enclave Boot monitor that's present on these platforms
* Replace USB backend with C **libanya** in the Python library - to improve performance of it
* Improve backend with ability of sending KBAGs in batch - to vastly improve performance when decrypting a big count of KBAGs

## Credits
* @axi0mX - for the idea of replacing USB handler (used in **ipwndfu**)
* @pimskeks and other people behind **libimobiledevice** project - for **libirecovery**
* @1nsane_dev - for a lot of tests on Cebu and Sicily
* dellaquila.federica (that's Instagram handle) - for the mascot
* People behind **pongoOS** - for SEP AES decryption algorithm 
* @matteyeux - for help with SEP support for Cyprus B1 and AP support for Cebu B0
