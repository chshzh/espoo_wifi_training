# nRF7120 DK Wi-Fi training

Build and run Wi-Fi samples on the **nRF7120 DK** (`nrf7120dk/nrf7120/cpuapp`) using the
nRF Connect SDK (NCS) **`main`** branch. Prebuilt firmware is included so you can flash first and build later.

| Item | Version used |
|---|---|
| NCS (`sdk-nrf`) | `main` @ `b58a49d545` (2026-10-02) |
| Toolchain | `v3.5.0-preview2` |
| `nrfutil device` | >= 2.21.0 (required for nRF71 programming) |

## 1. Prebuilt firmware

| Folder | Contents | Source |
|---|---|---|
| `firmware/wifi_shell_zperf/` | Wi-Fi shell with zperf (client and server) | `nrf/samples/wifi/shell` + `nrf71-zperf.conf` |
| `firmware/power_consumption/` | System ON idle power benchmark, all RAM retained | `nrf/samples/benchmarks/power_consumption` |

Each folder has `app.hex` and `uicr.hex`. **Flash both**, application first:

```sh
nrfutil device program --firmware firmware/wifi_shell_zperf/app.hex --x-family nrf71 \
  --options chip_erase_mode=ERASE_ALL,verify=VERIFY_READ
nrfutil device program --firmware firmware/wifi_shell_zperf/uicr.hex --x-family nrf71 \
  --options verify=VERIFY_READ
nrfutil device reset
```

Use `--serial-number <SN>` if more than one board is connected (`nrfutil device list`).
These standalone commands are untested here; if the board does not start, flash with `west flash`
from a build as described below.

## 2. Set up the NCS `main` workspace

### 2.1 Install tools

1. Install `nrfutil` and the `sdk-manager` command: <https://docs.nordicsemi.com/bundle/nrfutil/page/guides/installing.html>
2. Install a toolchain. `main` needs a recent one; `v3.5.0-preview2` worked:
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
  -DSB_CONFIG_WIFI_NRF70=n
```

The extra arguments match the `sample.nrf7120.shell.zperf` scenario in the sample's `sample.yaml`:
the nRF7120 Wi-Fi ROM patch overlay, `CONFIG_WIFI_NRF71_PATCH=y`, and disabling the nRF70 sysbuild image.
The prebuilt `app.hex` is `shell/zephyr/zephyr.nrf7120.hex` and `uicr.hex` is `uicr/zephyr/zephyr.hex` from this build.

### 3.2 Power consumption (System ON idle)

```sh
cd nrf/samples/benchmarks/power_consumption
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west build -p -b nrf7120dk/nrf7120/cpuapp -d build .
```

All RAM is retained by default. For other retention levels add a Kconfig option, for example
`-- -DCONFIG_SAMPLE_POWER_CONSUMPTION_RAM_RETAIN_64K=y` (also `128K`, `256K`, `512K`, `UNUSED_ONLY`).
The sleep time is `CONFIG_SAMPLE_POWER_CONSUMPTION_IDLE_SECONDS` (default 10 s).

For the non-secure variant use `-b nrf7120dk/nrf7120/cpuapp/ns`.

### 3.3 Flash from a build

```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.5.0-preview2 -- \
  west flash -d <build-dir> --dev-id <SN> --erase
```

## 4. Try it

- Serial console: 115200 8N1 on the DK's first VCOM port.
- Wi-Fi shell commands, TWT and zperf examples: [docs/wifi_commands.md](docs/wifi_commands.md).
- Power measurement with a PPK2: remove the **JP601** shunt, connect PPK2 Vout to **P601** pin 2 and GND to pin 3,
  set Source Meter mode at 3.6 V. See the sample's `README.rst` for details.
  Disable the UART console (`CONFIG_SERIAL=n`, `CONFIG_CONSOLE=n`) for the most accurate numbers.

## Notes

- `main` moves quickly. If a build breaks, check out the commit listed above.
- Other sample that measures System OFF: `nrf/samples/zephyr/boards/nordic/system_off`.
