# Firmware Versioning

This project uses two distinct versioning concepts that serve different purposes.

---

## Firmware Version (`DEVICE_FIRMWARE`)

The full firmware version string has the format:

```
MAJOR.PAIRING_CONFIG_ID.PATCH
```

Example: `1.12.1`

This value is stored in `src/pairing_config.h` as `DEVICE_FIRMWARE` and reported to HomeKit via the `FirmwareRevision` characteristic. It is what appears in the Home app under the accessory details.

- **MAJOR** — manually incremented for breaking changes; currently `1`
- **PAIRING_CONFIG_ID** — auto-incremented by `make generate-pairing` (see below)
- **PATCH** — auto-incremented by `scripts/commit.py` on every commit; resets to `0` when `PAIRING_CONFIG_ID` changes

---

## Pairing Config ID

`PAIRING_CONFIG_ID` is an integer defined in `src/pairing_config.h`. It increments each time `make generate-pairing` is run (via `scripts/generate_pairing.py`), which happens when the pairing QR code / setup code changes.

When `PAIRING_CONFIG_ID` increments, `PATCH` resets to `0`.

**This is the value displayed by the LEDs** during the factory reset warning animation — not the full firmware version string.

---

## Release Tags

Each verified, flashed build is tagged in git as:

```
release-0xHH
```

where `HH` is the hex representation of `PAIRING_CONFIG_ID`.

Example: `PAIRING_CONFIG_ID = 18` → tag `release-0x12`

These tags serve as stable reference points for:
- Matching a running device to the exact source code it was built from
- Looking up the pairing QR code that was active at that build (see `docs/img/pairing_qr.png` at the tag)

---

## LED Binary Display

During the factory reset warning animation, the first 8 LEDs display `PAIRING_CONFIG_ID` in binary:

- **Red LED** = bit 1
- **Blue LED** = bit 0
- LED 1 = MSB (bit 7), LED 8 = LSB (bit 0)

Use the [interactive decoder](https://outofjungle.github.io/homekit-matchstick-sputter/) to convert the LED pattern to a release tag.

---

## How `scripts/commit.py` Works

On every commit, `scripts/commit.py`:

1. Reads `PAIRING_CONFIG_ID` from `src/pairing_config.h`
2. Finds the most recent `release-0x*` git tag
3. If `PAIRING_CONFIG_ID` **has not changed** since that tag — bumps `PATCH` by 1, stages `src/pairing_config.h`
4. If `PAIRING_CONFIG_ID` **has changed** — skips the bump (patch was already reset to `0` by `generate_pairing.py`)
5. Creates the commit with the staged files

---

## How `scripts/generate_pairing.py` Works

`make generate-pairing` runs `scripts/generate_pairing.py`, which:

1. Generates a new random setup code and QR code
2. Increments `PAIRING_CONFIG_ID` in `src/pairing_config.h`
3. Resets `PATCH` to `0`
4. Saves the new QR image to `docs/img/pairing_qr.png`

After running, commit all changed files (including the QR image) together.

---

## Lifecycle Example

| Event | `PAIRING_CONFIG_ID` | `PATCH` | `DEVICE_FIRMWARE` | Git tag |
|---|---|---|---|---|
| Initial generate-pairing | 12 | 0 | `1.12.0` | — |
| Commit A (WIP) | 12 | 1 | `1.12.1` | — |
| Commit B (WIP) | 12 | 2 | `1.12.2` | — |
| Flash + verify | 12 | 2 | `1.12.2` | `release-0x0c` |
| New generate-pairing | 13 | 0 | `1.13.0` | — |
| Commit C (WIP) | 13 | 1 | `1.13.1` | — |
| Flash + verify | 13 | 1 | `1.13.1` | `release-0x0d` |
