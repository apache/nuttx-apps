# Bluetooth integration tests

Run NuttX Hosts against Python BSControllers through BabbleSim.

| Firmware | Commands | Checks |
|---|---|---|
| BTSak | `bt` | Discover the expected advertiser |
| NimBLE | `blecent`, `bleprph` | Discovery, connection, GATT discovery, subscription |

Each firmware runs as two processes with roles selected through NSH commands.
Configuration overrides: `configs/bthcisock.conf` and `configs/nimble.conf`.

## Run on Linux

Requires Linux/x86_64 and Python 3.12. Install the packages listed in
[bluetooth.yml](../../../.github/workflows/bluetooth.yml), and check out BSWorld
at its `BSWORLD_REVISION`.

From a directory containing sibling `nuttx`, `apps` and `bsworld` checkouts:

```sh
bash apps/testing/bluetooth/bsworld/prepare.sh \
  "$PWD/bsworld" "$PWD/.bluetooth-ci/dependencies"
bash apps/testing/bluetooth/bsworld/build.sh \
  "$PWD/nuttx" "$PWD/apps" "$PWD/.bluetooth-ci/dependencies" \
  "$PWD/.bluetooth-ci/build" "$PWD/bluetooth-results"

source .bluetooth-ci/dependencies/env.sh
export BSWORLD_NUTTX_BTSAK_BIN="$PWD/bluetooth-results/firmware/btsak"
export BSWORLD_NUTTX_NIMBLE_BIN="$PWD/bluetooth-results/firmware/nimble"
python -m pytest -c apps/testing/bluetooth/bsworld/pytest.ini \
  apps/testing/bluetooth/bsworld -v -n 2 -p no:cacheprovider \
  --out-dir "$PWD/bluetooth-results/runtime" \
  --junitxml="$PWD/bluetooth-results/bluetooth.xml"
```

`prepare.sh` installs dependencies. `build.sh` builds both images in source
copies. Set `BUILD_JOBS` to limit compilation parallelism.
To rerun tests, execute only the environment setup and pytest commands.
Use a new dependency directory when changing the BSWorld revision.

Results in `bluetooth-results/`: firmware/configs, build logs, JUnit report,
device logs, HCI snoops and Ellisys traces.
