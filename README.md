# nRF7120 DK Wi-Fi training

Build and run Wi-Fi samples on the **nRF7120 DK** (`nrf7120dk/nrf7120/cpuapp`) using the
nRF Connect SDK (NCS) **`main`** branch. Prebuilt firmware is included so you can flash first and build later.

| Item | Version used |
|---|---|
| NCS (`sdk-nrf`) | `main` @ `b58a49d545` (2026-10-02) |
| Toolchain | `v3.5.0-preview2` |
| `nrfutil device` | >= 2.21.0 (required for nRF71 programming) |

## 1. Prebuilt firmware

| File | Contents | Source |
|---|---|---|
| `firmware/wifi_shell_zperf_nrf7120dk.hex` | Wi-Fi shell with zperf (client and server) and `wifi cred` storage | `nrf/samples/wifi/shell` + `nrf71-zperf.conf` |
| `firmware/wifi_shell_zperf_nrf7002dk.hex` | Same Wi-Fi shell with zperf and `wifi cred`, for the **nRF7002 DK** (nRF5340 app core) | `nrf/samples/wifi/shell` + `overlay-zperf.conf` |
| `firmware/power_consumption_systemonidle64k_nrf7120dk.hex` | System ON idle power benchmark, 64 KiB RAM retained (as in the Confluence data-collection procedure), with the TRIM.LOWPOWER workaround | `samples/power_consumption` (copy of `nrf/samples/benchmarks/power_consumption`) |
| `firmware/nrf71_power_test_systemoff_nrf7120dk.hex` | System OFF power test: enters System OFF at boot (no wake-up source, no RAM retention, trim workaround applied) | `samples/nrf71_power_test` (from `simonduq/unified-test`, adapted to build on `main`) |

Each file is a complete image (the build's UICR image is empty, so nothing else needs flashing).

### Flash a prebuilt image

Use `nrfutil device` 2.21.0 or newer (needed for `--x-family nrf71`), run directly and not through the toolchain wrapper.
Replace `<firmware>` with one of the files above and `<SN>` with the serial number from `nrfutil device list`
(omit `--serial-number` if only one board is connected):

```sh
nrfutil device program --firmware firmware/<firmware>.hex \
  --x-family nrf71 \
  --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ \
  --serial-number <SN>
nrfutil device reset --serial-number <SN>
```

For the nRF7002 DK image, use the nRF53 family instead:

```sh
nrfutil device program --firmware firmware/wifi_shell_zperf_nrf7002dk.hex \
  --x-family nrf53 --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ \
  --serial-number <SN>
nrfutil device reset --serial-number <SN>
```

For example, the System OFF image:

```sh
nrfutil device program --firmware firmware/nrf71_power_test_systemoff_nrf7120dk.hex \
  --x-family nrf71 --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ
nrfutil device reset
```

`chip_erase_mode=ERASE_ALL` erases the whole chip first (like `west flash --erase`); drop it to keep other data.

For current measurements with the System OFF or System ON idle images, unplug the USB cable after flashing so the DK
runs only from the PPK2 (see section 4). The System OFF image prints nothing and only wakes on a pin reset or power cycle.

The standalone `nrfutil device program` commands have not been run on a board here; the equivalent `west flash`
(section 3.4) was. If the board does not start, flash from a build instead.

## 2. Set up the NCS `main` workspace

### 2.1 Install tools

1. Install `nrfutil` and the `sdk-manager` command: <https://docs.nordicsemi.com/bundle/nrfutil/page/guides/installing.html>
2. Update `nrfutil` and its `device` command. nRF71 programming needs `nrfutil device` 2.21.0 or newer:
   ```sh
   nrfutil self-upgrade
   nrfutil upgrade device
   ```
3. Install a toolchain. `main` needs a recent one; `v3.5.0-preview2` worked:
   ```sh
   nrfutil install sdk-manager
   nrfutil sdk-manager toolchain install --ncs-version v3.5.0-preview2
   ```

### 2.2 Get the sources

```sh
mkdir ncs_nrf7120 && cd ncs_nrf7120
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west init -m https://github.com/nrfconnect/sdk-nrf --mr main
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- west update
```

To switch an existing workspace to `main`:

```sh
git -C nrf fetch origin main
git -C nrf checkout -B main origin/main
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- west update
```

### 2.3 Fix the toolchain's `nrfutil device` (needed to flash nRF71)

The toolchain bundles its own `nrfutil-device` (2.20.0), which does not know the `nrf71` family.
Flashing fails with `invalid value 'nrf71' for '--family'`. The toolchain's nrfutil home is protected by a
`locked` file, so unlock it, upgrade, and lock it again:

```sh
TC=$(nrfutil sdk-manager toolchain list | awk '/v3.5.0-preview2/{print $2}')
cd "$TC/nrfutil/home"
rm locked
NRFUTIL_HOME=$PWD nrfutil install device=2.21.0 --force
touch locked
```

Check with `NRFUTIL_HOME=$PWD nrfutil list` (device should be 2.21.0 or newer).
Repeat this for every toolchain you install.

## 3. Build

Run from the workspace root. Replace the toolchain version if you use another one.

### 3.1 Wi-Fi shell with zperf

```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west build -p -b nrf7120dk/nrf7120/cpuapp -d nrf/samples/wifi/shell/build nrf/samples/wifi/shell --sysbuild -- \
  -DEXTRA_CONF_FILE=nrf71-zperf.conf \
  '-DEXTRA_DTC_OVERLAY_FILE=${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf71_wifi/bins/0.1.0/nrf7120_wifi_patch.dtsi' \
  -DCONFIG_WIFI_NRF71_PATCH=y \
  -DSB_CONFIG_WIFI_NRF70=n \
  -DCONFIG_WIFI_CREDENTIALS=y -DCONFIG_FLASH=y -DCONFIG_NVS=y -DCONFIG_SETTINGS=y -DCONFIG_SETTINGS_NVS=y
```

The first four extra arguments match the `sample.nrf7120.shell.zperf` scenario in the sample's `sample.yaml`:
the nRF7120 Wi-Fi ROM patch overlay, `CONFIG_WIFI_NRF71_PATCH=y`, and disabling the nRF70 sysbuild image.
`nrf71-zperf.conf` disables credential storage; the last line turns it back on so `wifi cred` works.
The prebuilt `firmware/wifi_shell_zperf_nrf7120dk.hex` is `shell/zephyr/zephyr.nrf7120.hex` from this build
(`firmware/power_consumption_systemonidle64k_nrf7120dk.hex` is `power_consumption/zephyr/zephyr.hex` from the next one).

For the nRF7002 DK (`sample.nrf7002.shell.zperf` scenario plus credential storage):

```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west build -p -b nrf7002dk/nrf5340/cpuapp -d nrf/samples/wifi/shell/build_nrf7002dk nrf/samples/wifi/shell -- \
  -DEXTRA_CONF_FILE=overlay-zperf.conf \
  -DCONFIG_WIFI_CREDENTIALS=y -DCONFIG_FLASH=y -DCONFIG_NVS=y -DCONFIG_SETTINGS=y -DCONFIG_SETTINGS_NVS=y
```

`firmware/wifi_shell_zperf_nrf7002dk.hex` is `shell/zephyr/zephyr.hex` from this build.

### 3.2 Power consumption (System ON idle)

This repo carries its own copy in `samples/power_consumption`: the `main` sample plus the
`TRIM.LOWPOWER` workaround described below. Build it from the workspace root:

```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west build -p -b nrf7120dk/nrf7120/cpuapp -d samples/power_consumption/build samples/power_consumption -- \
  -DCONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_64K=y
```

The in-tree sample (`nrf/samples/benchmarks/power_consumption`) builds the same way but without the workaround.

**TRIM.LOWPOWER workaround.** Some nRF7120 DKs have a factory trim on the 1.8 V LDO low-power comparator
(`VREGVBAT1V8` `TRIM.LOWPOWER` at `0x50126448`) that adds about 60 uA while sleeping. The copy writes `7` to it
right before each `k_sleep()` (secure builds only; it is not applied to `/ns`). Start-up code restores the factory
value, so it must be written just before sleeping. Per earlier measurements, one affected DK went from about
64.7 uA to about 3.3 uA. Good boards are not affected. To check your DK, build the in-tree sample without the fix:
about 60 uA means it is affected, a few uA means it is not.

The command above retains only the first 64 KiB of RAM, which is what the prebuilt hex uses. Without the option all RAM
is retained (the default). Other levels: `RAM_RETAIN_128K`, `256K`, `512K`, `UNUSED_ONLY`.
The sleep time is `CONFIG_SAMPLE_POWER_CONSUMPTION_IDLE_SECONDS` (default 10 s).

For the non-secure variant use `-b nrf7120dk/nrf7120/cpuapp/ns`.

### 3.3 System OFF (nrf71_power_test)

`samples/nrf71_power_test` is the System OFF sample from the Confluence procedure (`simonduq/unified-test`, commit `a837962bbc`).
Changes so it builds standalone on `main`: the radio_test Kconfig is copied locally as `Kconfig.radio_test`, and
`CONFIG_WIFI_NRF71_PATCH_VERSION` is removed from `prj.conf` (the patch comes from the devicetree overlay on `main`).

```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west build -p -b nrf7120dk/nrf7120/cpuapp -d samples/nrf71_power_test/build samples/nrf71_power_test -- \
  -DCONFIG_NRF71_POWER_TEST_DEFAULT_MODE_SYSTEMOFF=y \
  '-DEXTRA_DTC_OVERLAY_FILE=${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf71_wifi/bins/0.1.0/nrf7120_wifi_patch.dtsi'
```

Without the first option the image boots to a shell; run `systemoff` (or press `sw0`) to enter System OFF.
The sample writes `TRIM.LOWPOWER = 7` right before powering off. Only a pin reset or power cycle wakes it.

### 3.4 Flash from a build

```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west flash -d <build-dir> --dev-id <SN> --erase
```

## 4. Try it

- Serial console: 115200 8N1 on the DK's first VCOM port (on the nRF7002 DK, the second one).
- Wi-Fi shell commands, TWT and zperf examples: see section 5.
- Power measurement with a PPK2: remove the **JP601** shunt, connect PPK2 Vout to **P601** pin 2 and GND to pin 3,
  set Source Meter mode at 3.6 V. See the sample's `README.rst` for details.
  Disable the UART console (`CONFIG_SERIAL=n`, `CONFIG_CONSOLE=n`) for the most accurate numbers.

## 5. Wi-Fi shell commands

Replace `<SSID>` and `<PASSWORD>` with your own network. Do not commit real credentials.

```
wifi scan
wifi connect -s <SSID> -k 1 -p <PASSWORD>
wifi status
```

Optional: store the Wi-Fi credentials once, then connect with `wifi cred auto_connect` after a restart.

```
wifi cred add -s <SSID> -k 1 -p <PASSWORD>
wifi cred auto_connect
```

### Target Wake Time (TWT): 10 s wake-up, 65 ms awake

```
wifi twt quick_setup 65000 10000000
wifi twt setup -n 0 -c 0 -t 1 -f 0 -r 0 -T 1 -I 1 -a 1 -w 65000 -p 10000000 -D 0 -d 0
wifi twt teardown_all
```

### zperf

The PC side uses iperf2 (`iperf`, not `iperf3`). `<pc-ip>` is the PC address, `<dk-ip>` is the DK address.

Syntax:

```
zperf udp upload [-S tos -a] <dest ip> [<dest port> <duration> <packet size>[K] <baud rate>[K|M]]
zperf udp download [<port>] [<host>]
zperf tcp upload [-S tos -a -i sec -n] <dest ip> <dest port> <duration> <packet size>[K]
zperf tcp download [<port>] [<host>]
```

#### UDP upload (DK to PC)

```
# PC
iperf -s -u -i 1
# DK
zperf udp upload <pc-ip> 5001 10 1K 100M
```

#### UDP download (PC to DK)

```
# DK
zperf udp download 5001
# PC
iperf -c <dk-ip> -u -b 100M -l 1K -t 10 -i 1
```

#### TCP upload (DK to PC)

```
# PC
iperf -s -i 1
# DK
zperf tcp upload <pc-ip> 5001 10 1K
zperf tcp upload -n <pc-ip> 5001 10 1K
zperf tcp upload -a -i 1 <pc-ip> 5001 10 1K
```

#### TCP download (PC to DK)

```
# DK
zperf tcp download 5001
# PC
iperf -c <dk-ip> -t 10 -i 1
```

#### Stop the DK servers

```
zperf udp download stop
zperf tcp download stop
```

#### Reference results

DK uploading to a PC (iperf2) over an ASUS RT-BE92U, 5 GHz channel 165, Wi-Fi 6, NCS `main` @ `b58a49d545`,
prebuilt images from `firmware/`, 10 s runs with 1000-byte packets (`1k`):

| Board | UDP upload (`100M` requested) | TCP upload |
|---|---|---|
| nRF7120 DK | 52.8 Mbps, 0.1 % loss | 25.1 Mbps |
| nRF7002 DK | 19.2 Mbps, 0.03 % loss | 11.9 Mbps |

## Notes

- `main` moves quickly. If a build breaks, check out the commit listed above.
- Simpler alternative System OFF sample: `nrf/samples/zephyr/boards/nordic/system_off` (has wake-up options, no trim workaround).
