# IOTA Firmware Management Tool
`iota-cli` is a command-line tool designed for managing firmware on Linux devices. It supports operations such as checking out the current firmware and updating to a new firmware version.

# Usage

## checkout partitions
To checkout the current firmware partitions, use the following command:

```bash
# checkout the partition. The change will take effect after the next startup
iota-cli checkout

# checkout the partition and reboot immediately
iota-cli checkout --reboot --delay 0
```

## upgrade firmware
To upgrade the firmware to a new version, use the following command:

```bash
iota-cli upgrade \
    --firmware "$FIRMWARE_FILE" \
    --key "$HEX_KEY_STRING" \
    --verify "$PUBLIC_KEY_FILE" \
    --stream-count 51200 \
    --no-progress \
    --enable-dbus
```
