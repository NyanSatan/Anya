![](repo/anya_logo_with_girl.png)

*This is a deeply experimental branch of deeply experimental software, so be careful while using it and always validate results that it gives you*

**Anya** is a KBAG decryption kit for JTAGable prototypes

## About this branch

This branch (`future`) is designed to be far easier to compile and far faster to decrypt KBAGs than the legacy one (which is currently `main`)

You no longer need LLVM toolchain to build it, it is replaced with a modern **Xcode** and a little utility from Siguza - [vmacho](https://github.com/Siguza/misc/blob/master/vmacho.c)

And for speed look yourself - we will compare Apple M1 host against Cyprus B1 target (latest revision of Apple A12) with the legacy Anya:

```
noone@noones-MacBook-Air Anya % build/anyactl -b 10000
found: CPID:8020, CPFM:01, ECID:REDACTED
decrypting...
decrypted 10000 KBAGs in 10.008152 seconds, average - 999.185486 KBAGs/sec
```

...and the new Anya:

```
noone@noones-MacBook-Air Anya % build/anyactl -b 10000
found: CPID:8020 CPFM:01 ECID:REDACTED
decrypting...
decrypted 10000 KBAGs in 0.496268 seconds, average - 20150.402344 KBAGs/sec
```

And even though Intel hosts were faster with the legacy Anya, the new one still beats it even there. Speed-up may vary from 3x and up to 20x depending on the host+target combination

Speaking of targets - not everything is backported from the legacy one to the new as of now, but some new targets are now supported. The current full list is here:

### AP

* **Cyprus A0/B0/B1** - Apple A12
* **M9 B0/B1** - Apple S4/S5
* **Aruba A1** - Apple A12X/Z
* **Cebu A0/B0/B1** - Apple A13
* **Sicily A0/B0/B1** - Apple A14
* **Tonga B1** - Apple M1
* **Ellis A0/B0** - Apple A15
* **Staten B1** - Apple M2

### SEP

* **Cyprus A0/B1** - Apple A12
* **M9 B0/B1** - Apple S4/S5
* **Sicily A0/B1** - Apple A14
* **Ellis A0/B0** - Apple A15

## Building
### Requirements

* Modern Xcode
* Python 3
* libirecovery
* vmacho

### Building

Build system is still a dumpster fire, better just download a ZIP from Releases, but still you can try:

```
make
```

...or to build a package (basically, a ZIP):

```
make package
```

List of environmental variables you *might* need to provide:


* `ARM_CC` - ARM64 C compiler capable of producing Mach-Os. Xcode's Clang is used by default
* `ARM_OBJCOPY` - [vmacho](https://github.com/Siguza/misc/blob/master/vmacho.c), needed to extract raw code from a Mach-O
* `CC` - C compiler used to compile **anyactl** (client utility), by default it is Clang
* `PYTHON` - Python 3 interpreter used by some build scripts
* `VALID_HANDLER_TARGETS` - list of targets to build USB DFU handler for. Current list of valid targets is **Cyprus/A0**, **Cyprus/B0**, **Cyprus/B1**, **M9 B0/B1**, **Aruba/A1**, **Cebu**, **Sicily/A0**, **Sicily/B0**, **Sicily/B1**, **Tonga/B1**, **Ellis/A0**, **Ellis/B0** and **Staten/B1**

In the end you'll get a structure like this in the `build/` folder:

```
anya.ax
anyactl
libanya.dylib
payloads/
payloads/anya_handler.M9-B0_B1.bin
payloads/anya_handler.Cebu.bin
payloads/anya_handler.Ellis-A0.bin
payloads/anya_handler.Cyprus-B0.bin
payloads/anya_handler.Tonga-B1.bin
payloads/anya_handler.Cyprus-B1.bin
payloads/anya_handler.Ellis-B0.bin
payloads/anya_handler.Cyprus-A0.bin
payloads/anya_handler.Sicily-B1.bin
payloads/anya_handler.Aruba-A1.bin
payloads/anya_handler.Sicily-B0.bin
payloads/anya_handler.Staten-B1.bin
payloads/anya_handler.Sicily-A0.bin
python/
python/anyactl
python/anya/
python/anya/__init__.py
python/anya/exceptions.py
python/anya/c_api.py
python/anyafromjson
```

## How to use?
### Requirements

* Compiled Anya
* Astris (Azul tools or later)

### Usage

First of all, you need to put your device into Anya mode (basically SecureROM DFU with custom USB control request handler) via Astris:

```
ANYA_PAYLOAD=path/to/desired/payload astris --script path/to/anya.ax
```

***Warning**: this will force reset your device via `fromreset` Astris command! This will reset a SoC and catch it on the very first cycle. Other peripherals might be not so lucky though, so better put your device into iBoot recovery or SecureROM DFU mode before doing this! On devices with a display the DFU mode is strictly recommended, otherwise you'll see weird glitches on it or this may even potentially damage it!*

Upon successful execution you will get a log like this one:

```
noone@noones-MacBook-Air Anya % ANYA_PAYLOAD=build/payloads/anya_handler.Cyprus-B1.bin astris --script build/anya.ax
astris v2.7.4

WARNING: This version of tcl is included in macOS for compatibility with legacy software. 
In future versions of macOS the tcl runtime will not be available by 
default, and may require you to install an additional package.


Probe address: MORNINGSTAR
Probe type: koba
Probe firmware: 1.43
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
Uploading USB handler...
.
546 bytes sent in 0.087 sec, 6276 bytes per second
Forcing DFU...
bp 0 cleared
Patching iBoot flags...
bp 0 cleared
Overriding USB handler ptr...
bp 0 cleared
DONE!
Unloading SOC support script
noone@noones-MacBook-Air Anya %
```

Make sure **IBFL** (*iBoot Flags*) value in USB serial number has bit 6 set:

```
CPID:8020 CPRV:11 CPFM:01 SCEP:01 BDID:0E IBFL:6C ECID:REDACTED SRTG:[iBoot-3865.0.0.4.7]
```

This bit is not used by iBoot/SecureROM (except for macOS iBoot, apparently), so Anya sets it to indicate a device is in Anya mode

Starting from now on you can use `anyactl`, usage is quite straight-forward:

```
noone@noones-MacBook-Air Anya % build/anyactl
usage: build/anyactl ARG[s]

where ARG[s] must be one of the following:
        -k KBAG specifies KBAG to be decrypted
        -b NUM  runs benchmark with NUM random KBAGs
        -s      uses SEP GID (if possible)

you can also use this one with both of the above:
        -e ECID (hexa)decimal ECID to look for

noone@noones-MacBook-Air Anya %
```

Benchmark output example:

```
noone@noones-MacBook-Air Anya % build/anyactl -b 10000                                                              
found: CPID:8020 CPFM:01 ECID:REDACTED
decrypting...
decrypted 10000 KBAGs in 0.495929 seconds, average - 20164.175781 KBAGs/sec
noone@noones-MacBook-Air Anya %
```

KBAG decryption output example:

```
noone@noones-MacBook-Air Anya % build/anyactl -k 570375E980979CB973D5B761510B56E06A3DE617629CD46B97321E997A5D0560DE765F91805E61D1129A1E7954354815
found: CPID:8020 CPFM:01 ECID:REDACTED
991D32CE7633203427C8F26875C8538BB53F61EA6B0AD124E8EDBB5146F807C1BF5AC497D0DF66CE1994D7C5146D4570
noone@noones-MacBook-Air Anya %
```

***Warning**: since we're using prototype devices here, you obviously need to provide a development KBAG, not production (development one usually comes second in an Image4)!*

### SEP notes

In this branch we no longer execute code directly on **SEPROM**, as it's painful to set up and is straight out impossible on A13+ because of boot monitor. Instead we control SEP straight from AP cores via **CoreSight** - just like Astris is doing! Still only possible on Insecure devices, obviously

All the necessary code is already included in AP USB handler payload on supported platforms, so you no longer need to specify a path to SEP handler. That being said, on some platforms such as **Ellis**, you might need to also pass `ANYA_SEP_WARMUP=1` to the Astris script, otherwise AP might panic while accessing SEP

Now you are all set to decrypt SEP KBAGs, or to use a benchmark - just add `-s` flag to `anyactl`:

```
noone@noones-MacBook-Air Anya % build/anyactl -s -b 1000
found: CPID:8101 CPFM:00 ECID:REDACTED
decrypting...
decrypted 1000 KBAGs in 0.514751 seconds, average - 1942.686768 KBAGs/sec
noone@noones-MacBook-Air Anya % 
```

(Yes, unfortunately SEP mode is far slower than AP)


## Python
### Requirements
* Python 3.10+
* libanya

In the new Anya we no longer use `pyusb`. Instead, we use `libanya` as a backend, which is basically Anya's C library compiled as a dynamic library (powered by `libirecovery`)

`libanya.dylib` must be either in the current working directory, or you need to provide a path to it in `LIBANYA` environment variable. The variable overrides the current working directory's dylib

### Library usage

Placed in `python/anya` folder. Usage is quite straightforward:

```python
from anya import *
from anya.exceptions import *

dev = AnyaDevice(ecid=args.ecid) # creating connection object

try:
    dev.connect() # connecting
except AnyaError as e:
    print("failed to connect: %s" % str(e))
    exit(-1)

dev.print_device() # printing device CPID, CPFM and ECID

sep_supported = False

try:
    if not dev.ping_sep():
        print("SEP is unreachable")
    else:
      sep_supported = True
except AnyaError as e:
    print("failed to ping SEP: %s" % str(e))
    exit(-1)

try:
    decoded = decode_kbag(KBAG)	# decoding a KBAG string to bytes
except AnyaValueError as e:
    print("failed to parse KBAG: %s" % str(e))
    exit(-1)

try:
    key = dev.decrypt_kbag(decoded, sep=IS_SEP_NEEDED)	# decrypting a single KBAG
except AnyaError as e:
    print("failed to decrypt KBAG: %s" % str(e))
    exit(-1)

print(encode_key(key, to_upper=True)) # encoding key to a string (and printing)

#
# Decrypting multiple KBAGs
#

kbags = list()

for i in range(COUNT):
    kbags.append(bytes(token_bytes(KBAG_SIZE)))

try:
    dev.decrypt_kbags(kbags, sep=IS_SEP_NEEDED)
except AnyaError as e:
    print("failed to decrypt KBAG: %s" % str(e))
    exit(-1)

dev.disconnect() # disconnecting
```

### Tools usage

Placed in `python/` folder. There're 2 of them - **anyactl** (basically the same thing as the C variant of the ctl):

```
noone@noones-MacBook-Air Anya % LIBANYA=build/libanya.dylib build/python/anyactl 
usage: anyactl [-h] [-k KBAG] [-b COUNT] [-s] [-e ECID]

Decrypt some KBAG or run a benchmark

options:
  -h, --help  show this help message and exit
  -k KBAG     decrypt a KBAG
  -b COUNT    run benchmark
  -s          use SEP GID (if possible)
  -e ECID     ECID to look for
noone@noones-MacBook-Air Anya % 
```

...and **anyafromjson** - this one is used to decrypt KBAGs in batch from a JSON file:

```
noone@noones-MacBook-Air Anya % LIBANYA=build/libanya.dylib build/python/anyafromjson -h
usage: anyafromjson [-h] [-s] [-e ECID] in_path out_path

a dumb utility that takes KBAGs from an input JSON
and decrypts them with Anya. The input JSON must be
a list of dicts where every member must have "kbag"
field. The output JSON will be same, but with "key"
Thus, you can have arbitrary metadata in the dicts
that will be preserved in the output

positional arguments:
  in_path     input file
  out_path    output file

options:
  -h, --help  show this help message and exit
  -s          use SEP GID (if possible)
  -e ECID     (hexadecimal) ECID to look for
noone@noones-MacBook-Air Anya %
```

## TODOs

* Improve build system - for the current one is really bad
* Common offset database - so there won't be a need to duplicate some offsets/values in the Astris script and USB handlers configs

## Credits

* @axi0mX - for the idea of replacing USB handler (used in **ipwndfu**)
* @pimskeks and other people behind **libimobiledevice** project - for **libirecovery**
* @P5_2005 - for a lot of tests on the devices that I don't have
* dellaquila.federica (that's Instagram handle) - for the mascot
* People behind **pongoOS** - for SEP AES decryption algorithm

## Legacy credits

* @1nsane_dev - for a lot of tests on Cebu and Sicily
* @matteyeux - for help with SEP support for Cyprus B1 and AP support for Cebu B0
