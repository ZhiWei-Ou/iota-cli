# IOTAD

```bash
iota
 | - -V, --verbose              Enable verbose output
 | - -v, --version           Show the version information
 | - checkout
      |- -x, --script <script.sh>  Execute a custom script after checkout
      |- --reboot             Reboot the system after checkout
 | - update
      |- -i，--image <firmware.iota> Update the device firmware with the specified image
      |- --skip-checksum    Skip checksum verification during the update process
      |- --reboot      Reboot the system after the update
```

```iota
    [ firmwares... ]
        ↓ archive
    [ firmware.tar ]
        ↓ compress
    [ firmware.tar.gz ] 
        ↓ sign -> firmware.sig
    [ firmware.tar.gz ]
        ↓ encrypt (AES-GCM)
    [ firmware.enc ] 
        ↓ header
    [ firmware.iota ]

Header:
    - Magic Number (4 bytes) "IOTA"
    - DateTime (20 bytes) "YYYY-MM-DD HH:MM:SS"
    - Total Size (4 bytes)
    - IV (12 bytes)
    - Signature (256 bytes) ""
    - Reserved (12 bytes)

Workflow:
    Read Header
       ↓
    Decrypt firmware.enc
       ↓
    Obtain firmware.tar.gz
       ↓
    Verify Signature (header.signature vs firmware.tar.gz)
       ↓
    Pass? → Extract & Upgrade
    Fail? → Abort
```
