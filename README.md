![](repo/anya_logo_with_girl.png)

**Anya** is a KBAG decryption kit for JTAGgable iDevice prototypes - lets you decrypt **iBoot** & **SEP** firmwares that Apple keeps encrypting (for whatever reason)

**iBoot** is no longer encrypted since iOS 18.0 beta 4 (Summer 2025), **SEPOS** (and **SEPPatches**) still remain encrypted as of Autumn 2026

## SoC support
### AP

* **Alcatraz A0/B0** - Apple A7
* **M8 A5/B0** - Apple S1P/S2/T1
* **M8P A0/B0** - Apple S3
* **Gibraltar B0** - Apple T2
* **Skye A0** - Apple A11
* **Cyprus A0/B0/B1** - Apple A12
* **M9 B0/B1** - Apple S4/S5
* **Aruba A0/A1** - Apple A12X/Z
* **Cebu A0/B0/B1** - Apple A13
* **Sicily A0/B0/B1** - Apple A14
* **Turks A0/B0** - Apple S6/S7/S8
* **Tonga B1** - Apple M1
* **Ellis A0/B0/B1** - Apple A15
* **Staten B1** - Apple M2
* **Crete A0/B1** - Apple A16
* **Coll A0/B0** - Apple A17
* **Palma_1c A0** - Apple M3 Max (16-core)

### SEP

All the same as AP, **except**:

* **Cyprus B0**
* **Aruba A1**
* **Cebu A0/B0/B1**

Some platforms have all required offsets for SEP support, but it's disabled due to lack of testing:

* **Cebu A0/B0/B1** - Apple A13

## Changelog
<details>

### Anya-1.0.3

* Added Aruba A0 support

### Anya-1.0.2

* Fixed a bug where successful invocation of `Anya load` returned `-1` instead of `0`

* Fixed auto-detection for **M8**/**M8P** targets (Apple S1P/S2/T1/S3)

### Anya-1.0.1

* Fixed a stupid bug where running Anya without any arguments causes a segmentation fault

### Anya-1

* No longer a beta, I guess

* All the funcionality is now gathered under a single executable - `Anya`
    * Automatic detection of connected JTAG probe
        * If there are multiple - menu is shown
        * Overridable via command line argument

    * Astris script and USB DFU handler are automatically selected
        * Both scripts & handlers are embedded into the binary
        * Overridable via command line arguments, if needed

    * SEP warmup is now applied unconditionally

    * JSON KBAG batch decryption is also implemented there

    * So is benchmark

    * Python tools are deprecated, but still included

### Beta 13

* TBM platforms are now supported by the main Anya script (`anya.ax`)
    * DFU-only devices can work as well now
    * The bypass is provided by anonymous contributor
    * The old TBM script (`anya_tbm.ax`) was removed

* **Coll B0** support

### Beta 12

* Enabled **Sicily B0** SEP support

* Added support for **M8 B0** & **M8P B0** support (latest revisions of Apple S1P/S2/T1 & S3)

### Beta 11

* AP TBM platforms (Apple A16 & A17) support is now implemented in a unified script - `anya_tbm.ax`
    * `anya_crete.ax` & `anya_coll.ax` scripts were removed

* Added **Palma_1c A0** (Apple M3 Max, 16-core) support
    * Implemented in the `anya_tbm.ax` script as well

* Added **M8 A5** & **M8P A0** support (Apple S1P/S2/T1 & S3) - just for the sake of their SEP
    * Use `anya_v7.ax` script for these platforms

* Little improvements here and there in the control utilities

### Beta 10

* Added **Gibraltar B0** support

* Added **Alcatraz A0/B0** support
    * Use `anya_4k.ax` script for these platforms. You might also need older Astris

* Added **Coll A0** support
    * Also B0, but it's untested
    * Use `anya_coll.ax` script for this platform

* Switched to `lilirecovery` from original `libirecovery`

* Little improvements here and there in control utilities

### Beta 9

* Added fallback operations in `anya_crete.ax` for missing/broken ones in Sky tools' Astris

### Beta 8

* Added **Crete A0/B1** support
    * Pay attention to use `anya_crete.ax` script for these platforms

* Added **Turks A0/B0** support

* Added **Ellis B1** support

* Enabled SEP support for **Tonga B1** & **Staten B1**

### Beta 7

Major rework

This version (originally was developed in branch `future`) is designed to be far easier to compile and far faster to decrypt KBAGs than the legacy one (which used to be `main` and now is `legacy`)

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


### Beta 6

* Added support for the following SoCs:
    * **Cebu B0**
    * **Tonga A0**
    * **Turks B0**

### Beta 5

* Initial release with SEP support for **Cyprus B1** and **M9 B0/B1**

### Beta 4

* Support for **Cyprus B0**

### Beta 3

* Support for **Sicily A0**

### Beta 2

* Python API & tools initial release

### Beta 1

* Initial release. The following SoCs are supported:
    * **Cyprus B1**
    * **M9 B0/B1**
    * **Cebu B1**

</details>


## How to use?
### Requirements

* Compiled Anya
* Astris (Sky tools or later)
    * Certain newer platforms require newer Astris (Apple A17+)

### Usage
#### Initialization
First of all, you need to put your device into Anya mode (basically SecureROM DFU with a custom USB control request handler):

```
>  Anya load
```

***Warning**: this will force reset your device via `fromreset` Astris command! This will reset a SoC and catch it on the very first cycle. Other peripherals might be not so lucky though, so better put your device into iBoot recovery or SecureROM DFU mode before doing this! On devices with a display the DFU mode is strictly recommended, otherwise you'll see weird glitches on it or this may even potentially damage it!*

Upon successful execution you will get a log like this one:

```
>  Anya load
Anya-1-dirty
made by john (@nyan_satan)

Exploring IDE in JTAG
Detected Ellis A0

============ Launching Astris ============
astris v2.18.0

...

Probe address: DROPOFEVIL
Probe type: kanzi
Probe firmware: 1.43
Kanzi cable is not supported on this device. Please use a Koba. Astris connection may drop.
Probe tckrate: 977800

Exploring IDE in JTAG
Listening on port 8000 for ECORE0, ECORE1, ECORE2, ECORE3, PCORE0, PCORE1
Listening on port ** Open additional ports with 'gdbserver add <CPU>' **
Detected Ellis A0
Loading SOC support script
Identified product D64AP
Fromresetting device...
ECORE0: ASTRIS_ERR_CPUNOTHALTED
ECORE1: ASTRIS_ERR_CPUNOTHALTED

...

Forcing DFU...
bp 0 cleared
Dealing with MMU...
bp 0 cleared
Uploading USB DFU handler...
.
2352 bytes sent in 0.041 sec, 57366 bytes per second
Patching iBoot flags...
bp 0 cleared
Overriding USB handler ptr...
bp 0 cleared
Warming up SEP...
SEP: ASTRIS_ERR_OK
DONE!
Unloading SOC support script
============ Astris finished ============

Anya initialization finished!
took - 4.35s
```

Make sure **IBFL** (*iBoot Flags*) value in USB serial number has bit 6 set:

```
SDOM:01 CPID:8110 CPRV:00 CPFM:00 SCEP:00 BDID:0E ECID:REDACTED IBFL:64 SRTG:[iBoot-6338.0.0.200.15]
```

This bit is not used by iBoot/SecureROM (except for macOS iBoot, apparently), so Anya sets it to indicate a device is in Anya mode

***Important note for those who uses precompiled releases**: I obviously do not have Apple developer account to properly sign the tools and libraries, so in case you cannot run them due to quarantine restrictions, you can fix it by running `xattr -cr` against the root of the uncompressed archive*

#### Decrypting

Starting from now on you can decrypt KBAGs:

```
>  Anya kbag C6CF170574DDC126F0447C00D8A678B3DC0C39157E3851ED0A9339D103A6213A69F14A2AEF97D573F3B5D4CE7BA89C64
Anya-1-dirty
made by john (@nyan_satan)

CPID:8110 CPFM:00 ECID:REDACTED
713F138E0E8DB5BE96E6539E40CE8348AD0651A04D86DBA2336369F3EAFF1BF52AAAF7C76DEFEF425FAA27FAC24B642A
```

Add `-s` flag to decrypt with SEP GID:

```
>  Anya kbag 713F138E0E8DB5BE96E6539E40CE8348AD0651A04D86DBA2336369F3EAFF1BF52AAAF7C76DEFEF425FAA27FAC24B642A -s
Anya-1-dirty
made by john (@nyan_satan)

CPID:8110 CPFM:00 ECID:REDACTED
will use SEP GID
2EF1632F2A2430188ABAC919AA272425F78D5A2FCA8F55358F08372119BC8B67E6179055B7668E4E85AE28BDAD99C980
```

***Warning**: since we're using prototype devices here, you obviously need to provide a development KBAG, not production (development one usually comes second in an Image4)!*

#### Mass decrypting

Anya supports what I call "batches". A "batch" is an array of dictionaries encoded to a JSON file. Each dictionary must have `"kbag"` field. After processing with Anya, you will get the same JSON file, but each entry will be populated with `"key"` field. Entries are allowed to contain any other fields that you can use for identification or metadata. They are preserved as is by the program

```
    [
            {
                    "kbag": "KBAG",
                    "metadata_1": "METADATA",
                    ...
                    "metadata_n": "METADATA"
            },
            ...
    ]
```

```
>  Anya batch A15/sep.json A15/sep.dec.json -s
Anya-1-dirty
made by john (@nyan_satan)

CPID:8110 CPFM:00 ECID:REDACTED
will use SEP GID
successfully decrypted 115 KBAGs in 0.060 seconds (1920.668 KBAGs/sec)
```

#### Benchmark

Used for debugging of Anya and performance tests. Decrypts N amount of random KBAGs and calculates how much time it took and average speed (KBAGs per second)

```
>  Anya bench 10000
Anya-1-dirty
made by john (@nyan_satan)

CPID:8110 CPFM:00 ECID:REDACTED
decrypting...
decrypted 10000 KBAGs in 0.643 seconds, average - 15541.706 KBAGs/sec
```

### Manual initialization

Anya-1 (and later) does its' best to recognize connected device and initialize it accordingly. However, you can still do it manually by calling Astris with `--script` argument

```
ANYA_PAYLOAD=path/to/desired/payload ANYA_SEP_WARMUP=1 astris --script path/to/anya.ax
```

For certain platforms, you might need to use a different script:

* **Alcatraz** - `anya_4k.ax`

* **M8** & **M8P** - `anya_v7.ax`

You can also pass custom scripts and payloads to Anya through command line args

### SEP notes

Nowadays we do not execute code directly on **SEPROM**, as it's painful to set up and is straight out impossible on A13+ because of boot monitor (TBM). Instead we control SEP straight from AP cores via **CoreSight** - just like Astris is doing! Still only possible on Insecure devices, obviously

The control tool accepts `-s` argument for all relevant verbs. It switches to SEP GID

***Important note**: Astris needs to be NOT running if you want to use SEP GID, as it will interfere*

## Building
### Requirements

* Modern Xcode
* Python 3
* lilirecovery
    * Included as a Git submodule
* [vmacho](https://github.com/Siguza/misc/blob/master/vmacho.c)

### Building

Build system is still a dumpster fire, better just download a tarball from Releases, but still you can try:

```
make
```

...or to build a package (a tarball):

```
make package
```

List of environmental variables you *might* need to provide:

* `ARM_CC` - ARM64 C compiler capable of producing Mach-Os. Xcode's Clang is used by default
* `ARM_OBJCOPY` - [vmacho](https://github.com/Siguza/misc/blob/master/vmacho.c), needed to extract raw code from a Mach-O
* `CC` - C compiler used to compile Anya's client utility, by default it's Clang
* `PYTHON` - Python 3 interpreter used by some build scripts

In the end you'll get a structure like this in the `build/` folder:

```
Anya
libanya.dylib
astris/anya_v7.ax
astris/anya_4k.ax
astris/anya.ax
payloads/Anya.Aruba-A1.bin
payloads/Anya.Staten-B1.bin
payloads/Anya.Cebu.bin
...
payloads/Anya.M8-A5.bin
payloads/Anya.Ellis-A0.bin
payloads/Anya.Tonga-B1.bin
payloads/Anya.M8P-B0.bin
python/anyactl
python/anya
python/anyafromjson
python/anyatest
...
```

## Python

Python bindings are still included, but now deprecated since batch KBAG processing is now implemented in the main C control code

If you still want to use them for some reason, please let me know

## Credits

* @axi0mX - for the idea of replacing USB handler (used in **ipwndfu**)
* @pimskeks and other people behind **libimobiledevice** project - for **libirecovery**
* @P5_2005 - for a lot of tests on the devices that I don't have
* chenurn - for Palma_1c/A0 bring-up tests
* People behind **pongoOS** - for SEP AES decryption algorithm
* dellaquila.federica (that's Instagram handle) - for the mascot art

## Legacy credits

* @1nsane_dev - for a lot of tests on Cebu and Sicily
* @matteyeux - for help with SEP support for Cyprus B1 and AP support for Cebu B0
