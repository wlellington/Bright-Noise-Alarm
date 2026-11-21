# Bright Noise Alarm

Custom light and audio alarm clock based on an Adafruit Feather RP2040 PropMaker.
This workspace is being prepared for a rewrite; the tracked firmware is the old
GitHub snapshot, not the new implementation.

## Local Arduino environment

The local Arduino CLI, RP2040 core, libraries, and build cache live in
`.arduino/`, which is not tracked by Git. Run commands from the repository root
so the relative paths in `arduino-cli.yaml` resolve correctly:

```sh
.arduino/bin/arduino-cli --config-file arduino-cli.yaml core list
.arduino/bin/arduino-cli --config-file arduino-cli.yaml lib list
```

The target board is `rp2040:rp2040:adafruit_feather_prop_maker`. The legacy
hardware-test sketch is maintained separately in
`../(OLD)Bright-Noise-Alarm-Working`; that checkout contains uncommitted work and
must remain untouched.

The installed CLI is version 1.6.0 and the RP2040 core is 6.4.0. To install
the same direct dependencies using an existing Arduino CLI, run these commands
from the repository root:

```sh
arduino-cli --config-file arduino-cli.yaml core update-index
arduino-cli --config-file arduino-cli.yaml core install rp2040:rp2040@6.4.0
arduino-cli --config-file arduino-cli.yaml lib install \
  "Adafruit NeoPixel@1.15.5" "Adafruit NeoMatrix@1.3.3" \
  "Adafruit LED Backpack Library@1.5.1" "Adafruit seesaw Library@1.7.9" \
  "Adafruit LIS3DH@1.3.0" "RTClib@2.1.4" "SdFat@2.3.0" \
  "BackgroundAudio@1.3.3" "ESP8266Audio@2.4.3"
```

The CLI also installs transitive dependencies; their versions are not locked
by these commands. On this machine, an isolated copy of the legacy hardware
test compiled for the target with the versions above. This does not verify
operation on the physical hardware.

The project uses `main`, `dev`, and feature branches. Requirements and rewrite
design are still to be developed; no firmware has been flashed from this setup.
