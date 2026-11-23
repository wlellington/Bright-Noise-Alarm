# Bright Noise Alarm

Custom light and audio alarm clock based on an Adafruit Feather RP2040 PropMaker.
The firmware rewrite is in progress on feature branches and is not integrated.
Menu navigation was physically verified; clock, alarm and sleep edits now save
automatically, with native encoder diagnostics and reboot checks passing. The hour/minute
flashing display and USB serial set-time/error/startup-gate checks also pass.
Light-first alarm menus, scheduling and restart recovery pass device checks.
SD WAV streaming and I2S DMA run on core 1, with serial controls on core 0.
Wall-power-only startup verification is deferred until that power setup is
available.

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

Run the encoder/menu state-machine tests with `make -C tests test`. Compile the
firmware with:

```sh
.arduino/bin/arduino-cli --config-file arduino-cli.yaml compile \
  --fqbn rp2040:rp2040:adafruit_feather_prop_maker Firmware/Light_Clock_Firmware
```

Uploading firmware requires the Feather to be in programming mode. Confirm
that mode before any upload.

The main menu includes a selectable `Back` item and scrolls longer labels
through the four-digit display using the legacy seven-segment glyph map.
Under Settings, `Set time` edits year, month, day, hour, minute and second
with the encoder. Each adjustment writes the RTC immediately; pressing the
encoder advances to the next field, then returns directly to Settings after
seconds. Shorter months and non-leap years clamp the day to a valid date.
Unchanged time fields use the live RTC when adjusting another field, so
time spent navigating does not rewind the clock.
The front button remains unwired.
Alarm, sleep timer, clock and system settings have no Save step: each valid
adjustment is saved automatically without a success screen interrupting editing.
Choosing a sound or toggling an alarm day saves immediately too.
Back and the 60-second menu timeout return without undoing saved changes.
Invalid settings and failed saves show an error; failed alarm/sleep edits revert
to the last confirmed settings. For a light-only sleep slot, choose a sound
before enabling Audio only or Both. A sleep slot with audio enabled cannot
select None until its Outputs is Lights only.
Running light/audio programs are not restarted by saving their next-use settings.
Sleep retains its shorter configured interaction wake window (default 30 seconds).
Host tests cover the exact 59,999/60,000 ms boundary, activity resets, timer
wraparound, error screens and pending-action cancellation. Earlier idle checks
verified draft cancellation and continued playback; saved adjustments now remain
in effect instead of requiring a later Save. Standalone Live lights controls
remain temporary and do not gain SD preset persistence.

### Automatic-save verification (2026-11-22)

All seven strict host executables and the menu address/undefined-behavior
sanitizer run passed. Host tests exercise every alarm/sleep scalar editor,
sound/day selection, clock fields and calendar boundaries, reload/idle
retention, invalid combinations and failed-save feedback.
Native encoder diagnostics changed Alarm 5 volume from 5 to 4 and Sleep 3 Hold
from 24 to 23, without selecting Save or leaving their value editors. Both
changes survived a firmware reboot. Clock seconds changed immediately from
22:33:00 to 22:33:01; clicking seconds returned directly to Settings.
All five alarms, three sleep slots, the original sleep draft and display/dark
settings were restored and compared unchanged. Playback remained stopped.
The uploaded build uses 156,464 program bytes and 23,604 bytes of static RAM.

## Alarms (light-first increment)

Alarms contains five slots (`ALM1`-`ALM5`), then Snooze, Dismiss, Dim Half and Back.
Select a slot to edit Enabled, Hour, Minute, One Shot, Days, Effect, Fade In,
Hold, Fade Out, RGBW and Brightness. In Days, click a day
to toggle it. Changes save as they are made; press the encoder to finish a field.
Back keeps the edits. Start alarm starts that slot's saved
configuration immediately, even when disabled, without consuming a one-shot.

New slots are disabled, at 07:00 on every day, with Sunrise, a 30-minute fade-in
and 30-minute hold. The configured time is the **peak**: a 07:00 alarm with a
30-minute fade-in starts at 06:30. Hold is the time at peak before fade-out
begins, independently selectable from 0 to 180 minutes. Total duration is
Fade In + Hold + Fade Out. Fade choices match Light Effects below.
One-shots disable durably when their scheduled ramp starts. Simultaneous
starts choose the lowest-numbered slot; a new occurrence preempts previous
outputs, which do not resume afterward.

Snooze immediately blanks the panel and stops audio for five minutes, then
continues the paused animation and audio position. The clock alternates time
and SNZ while snoozed. Dismiss stops both outputs; Dim Half halves light and
audio gain.
The software front-button path provides short-release snooze, a `HLD2`
reminder and two-second dismissal, but physical verification awaits wiring.
Physical tap/potentiometer integration is not part of this increment.

Alarm settings and occurrence markers use separate CRC-checked alternating
`BNAL0.DAT` and `BNAL1.DAT` records, leaving display and legacy files untouched.
An interrupted replacement can recover from the older valid record.
Restart recovers an occurrence that would still be running at the correct
elapsed position; expired, dismissed, preempted or snoozed occurrences do not
restart. Manual tests do not restart. Clock changes do not start an already
active window, and correcting the clock back does not block future schedules.
Edits affect future occurrences; disabling an enabled slot stops its active run.
An already-running occurrence otherwise retains its original output settings.

Storage errors are explicit and block configuration writes/new starts until
restart. Stop/snooze still blank the outputs on a write failure, but failed
storage cannot guarantee that the occurrence marker will survive a restart.
Each alarm saves a WAV path, peak volume and two shared fade settings:
Fade In controls both light and audio rising, and Fade Out controls both falling.
`Sound` opens a root-directory picker with None, up to twelve WAV files, and
Back; additional/subdirectory files can be selected over serial. Paths must
fit in 95 characters; longer names need renaming. WAV compatibility is checked
on core 1 when played. Light begins at peak minus Fade In. Audio waits through
the first half of that period, then fades in during the second half so both
reach peak together. With Fade In set to five minutes, light starts at 0:00,
audio starts at 2:30, and both reach peak at 5:00. Zero starts both immediately.
Audio loops without EOF silence until its fade-out ends. Both outputs hold
their peaks for Hold minutes. During Fade Out, audio fades to silence in the
first half; light fades across the entire period. For 30-minute Fade In,
10-minute Hold and 10-minute Fade Out: light rises 0–30, audio rises 15–30,
both hold 30–40, audio falls 40–45 and light falls 40–50 minutes.
Animated alarm effects keep moving at peak and throughout fade-out; the fade
scales each newly rendered frame rather than dimming a frozen snapshot.
The frozen-frame regression was reproduced and fixed on 2026-11-22. Host tests
cover Forest, Beach, Fire and Nebula through peak, mid-fade and expiry; the
active Forest device test resumed after upload and verified moving pixels
through its remaining fade-out without changing its scheduled end.
Missing/invalid audio stops only sound: the configured light continues and
the clock alternates time with `AErr`. Underruns also produce a warning and
`AErr` while alarm audio is playing. Version-1 through version-3 alarm records migrate
on the next save to 1024-byte version-4 records. Existing light fade durations
become the shared durations, including in active restart snapshots.
Version-4 records store Hold independently. Versions 1–3 migrate old Run to
Hold = Run minus Fade Out, including the active restart snapshot, preserving
the light schedule and end time. `SET_ALARM n HOLD minutes` sets Hold.
Legacy `RUN` remains a post-peak-duration alias: setting it derives Hold
from the current Fade Out; getters report both Hold and legacy Run.
Version-1 records retain **no selected sound**, so migration does not
unexpectedly play audio. Active sound settings are included
in the immutable restart snapshot. The low LED-output cap is unchanged.
An audio read failure during a running alarm does not stop its in-memory light
program. A cold start without the SD card cannot load the saved alarm bank:
it reports SD Error and blocks normal alarm starts. Cold-start configuration
availability without SD remains a separate limitation.

On 2026-11-22, host tests and the RP2040 build passed. USB serial device checks
verified RTC set/read, five independent slot settings, encoder editing/Save/Test, simultaneous
one-shot priority, starts within one second, a real one-minute ramp to peak,
mid-ramp restart recovery, durable one-shot disabling, no expired replay,
alternating time/SNZ, a real five-minute snooze and no restart while snoozed.
The final uploaded build also passed the forward/backward clock-correction
regression, editing a triggered one-shot without changing its active output,
and saved test/half/immediate-snooze/dismiss checks. All five saved configurations
and the advancing RTC were restored, with LEDs off. These tests read commanded
frames; physical front-button and abrupt SD-write power-cut tests remain pending.

## SD audio streaming (core 1)

Core 1 owns the audio file handle, WAV validation, PCM conversion, volume scaling,
I2S setup, DMA interrupts and playback lifecycle. Core 0 sends requests through
an eight-entry queue and reads a mutex-protected status snapshot; it does not
decode or feed audio. System SD initialization stays on core 0. All SD operations,
including settings writes, directory diagnostics and core-1 audio reads, share
one filesystem instance protected by a recursive mutex.

The reader accepts little-endian RIFF/WAVE integer PCM (format 1), mono or
stereo, 8/16/24/32-bit, at 8,000–96,000 Hz. The I2S output remains **16-bit**:
24/32-bit inputs discard their low bits; accepting a wider input does not
increase output resolution. Float, compressed and WAVE_FORMAT_EXTENSIBLE
files are not supported. Not every accepted format streams reliably at the
default SPI clock; measured limits are below. Metadata and padding are skipped;
header scanning is bounded to 64 chunks. Unsupported or malformed formats
fail before playback. Audio streams through bounded PCM blocks and sixteen
256-frame I2S buffers rather than loading the file into RAM. EOF commits the
partial last buffer and waits for DMA to finish, with a bounded timeout.

The current commissioning volume limit is **50% digital sample gain**, raised
from 20% at the user's request for further testing:
requests outside 0–50 are rejected, not clamped. Saved volume settings are
unchanged. Sound starts automatically
only for an explicitly started sleep timer or an alarm configured with a selected file.
This is not an acoustic loudness/power measurement or calibration of the
amplifier's verified maximum; REQ-059's calibrated limit remains pending.

The 50% cap firmware was uploaded on 2026-11-22. Device checks confirmed
`LIMIT 50`, accepted a 50% volume request while stopped and rejected 51%.
No playback was started. Saved alarms, sleep slots, display settings and the
Ocean draft were preserved. All seven host test executables and the native
firmware build passed, including menu wraparound and PCM scaling at 50%.

Live-volume verification: the Ocean/Beach test finished naturally at its
unchanged 5% peak with zero underruns; firmware was uploaded only afterward.
Device checks accepted 20% during an active sleep rise and a waiting alarm,
preserving ownership and timing, rejected 21%, and still blocked unrelated
manual starts. The sleep check used only the quiet initial rise, not a
full-volume listening test. All five saved alarms and three sleep slots were
unchanged; the Ocean draft was restored with playback stopped.
Host regressions, ASan/UBSan and the native firmware build passed.

```text
START_AUDIO 5 /Ooblets_32.wav
SET_AUDIO_VOLUME 0
STOP_AUDIO
GET_AUDIO
```

Start syntax is `START_AUDIO percent /path/to/file.wav`.
`SET_AUDIO_VOLUME percent` also adjusts the active alarm/sleep playback's peak
without restarting it or changing its fade timeline. It does not modify saved
settings; later starts or snooze resumes use the configured volume. It reports
BUSY while an alarm/sleep audio start is still awaiting submission, so a
pending start cannot undo an acknowledged adjustment.
Other manual playback starts remain blocked while those modes own outputs.
Absolute SD paths may contain spaces and must fit in 95 characters. Successful control responses
are `OK <command> QUEUED id`: this acknowledges the request, not successful
playback. `GET_AUDIO` reports the last processed request and its result,
`STOPPED|WAITING|PLAYING|ERROR`, configured peak volume/limit, format, frame/byte progress,
underruns and sample peaks. Short files may already be stopped at EOF when
queried. Errors identify `FILE`, `FORMAT`, `READ`, `OUTPUT`, `UNAVAILABLE`,
`INVALID_VALUE` or queue `BUSY`; transitions also produce `EVENT AUDIO` lines.
`CORE` identifies the worker; `IRQ_CORE` verifies the observed DMA interrupt
core (255 before any interrupt). An interrupt on the wrong core is latched
and causes an output error.

Volume changes apply to newly generated blocks; already buffered samples drain
first. Stop discards queued playback once core 1 processes it. `GAIN_BPS`
reports the last block's effective gain (500 means 5%), including the alarm
envelope and Dim Half. The envelope is calculated on core 1 and compensates
for queued I2S frames. `PEAK` is the
session maximum scaled sample magnitude; `BLOCK_PEAK` describes the last
submitted block, not a microphone reading. A new alarm replaces manual audio
with its own sound, or stops it if Sound is None. Manual starts, volume changes
and benchmark requests report BUSY during an alarm. `STOP_AUDIO` is allowed:
it silences only sound for that occurrence while the light continues; it does
not dismiss the alarm. Snooze/resume starts the saved sound again.
Preempted manual audio does not resume.

On 2026-11-22, host tests and the RP2040 build passed. Device tests streamed
327,680 bytes from the 12 MB, 32 kHz mono WAV silently in five seconds, exceeding
the board's entire RAM. The owner confirmed audible playback of that file.
Tests used 0%, 5%, a 10% boundary check, and three 44.1 kHz stereo clicks at 1%.
The final build verified both worker and DMA interrupt on core 1, full click
frame counts through EOF, mute, missing/invalid-file errors and alarm preemption.
Settings writes, RTC/screen queries, forest animation and SD listing ran with
zero audio underruns; the measured combined settings/time/screen round trip
peaked at 56 ms. Audio stopped and LEDs cleared afterward; display settings,
five saved alarms and SD audio files were preserved. Saved standalone audio
presets remain pending; sleep integration is described below.

### Alarm audio integration verification

On 2026-11-22 the final firmware passed six host executables, address/undefined
behavior sanitizers, and a Feather build (141,112 bytes program; 22,900 bytes
static RAM). Device verification used at most **5%** configured gain; throughput
and odd-sized-WAV tests were muted. Worker and observed DMA interrupts stayed
on core 1.

The native encoder picker selected and saved the existing Ooblets WAV. A
scheduled stereo sine fixture produced 469 samples across a real one-minute
fade-in and one-minute fade-out, with zero underruns across repeated loops.
Near the configured peak the reported gain was 495–499/500 basis points and
the commanded white output reached its 15/255 cap; both were off at the
scheduled end. This verifies software output timing, not acoustic or optical
measurements. Host tests also check the exact 06:30 start/07:00 peak for
30-minute ramps. Those initial independent audio times were subsequently
replaced by the shared fade settings described above.

Dim Half changed effective audio gain from 500 to 250 basis points and white
output from 15 to 7. Manual start/volume commands returned BUSY during the
alarm; STOP_AUDIO silenced only sound. Queue-backpressure tests ensure a
pending alarm start cannot subsequently undo an accepted manual stop.

A second scheduled alarm used the existing stereo click WAV at 3%. Restart
mid-ramp recovered its elapsed gain and light level, retaining that sound and
volume despite changes to its already-disabled one-shot's saved configuration.
A real five-minute snooze stopped both outputs and resumed at the same
199-basis-point gain and audio snapshot. Another snooze followed by restart
did not replay. A missing selected WAV reported FILE and alternating AErr/time
while the configured white light continued, without repeated start attempts.
Additional muted 8/24-bit mono fixtures at 8,001 Hz verified odd-byte RIFF
padding and exact 8,001-frame EOF completion.

Temporary fixtures were removed, all five original alarm configurations and
display settings were restored, and the RTC was advanced from its original
value by elapsed test time. SD audio filenames/sizes were preserved; only the
reserved alarm records intentionally grew from 256 to 1024 bytes. Default SPI
was restored to 10 MHz; audio and LEDs were left off. Physical tap/front-button
checks, cold-start operation without SD, and calibrated output
limits remain pending.

### Audio throughput experiments (2026-11-22)

Thirty five-second nonzero PCM fixtures were streamed muted, with RTC/screen
queries during playback, checking exact input frame/byte totals and core-1 DMA.
Fixtures used exclusively created `/BNTESTxxxxxxxx` directories and were
deleted after each case. All SD writes stayed at 10 MHz; 20 MHz comparisons
used only read-only playback. The default remains **10 MHz**.

| Input format | Required bytes/s | Original 1 ms delay, 10 MHz underruns | Adaptive worker, 10 MHz | Adaptive worker, 20 MHz |
| --- | ---: | ---: | ---: | ---: |
| 8 kHz, mono, 8-bit | 8,000 | 0 | 0 | 0 |
| 32 kHz, mono, 16-bit | 64,000 | 0 | 0 | 0 |
| 44.1 kHz, stereo, 16-bit | 176,400 | 0 | 0 | 0 |
| 48 kHz, stereo, 16-bit | 192,000 | 0 | 0 | 0 |
| 48 kHz, stereo, 24-bit | 288,000 | 0 | 0 | 0 |
| 48 kHz, stereo, 32-bit | 384,000 | 0 | 0 | 0 |
| 88.2 kHz, stereo, 16-bit | 352,800 | 409 | 1 | 0 |
| 96 kHz, stereo, 16-bit | 384,000 | 654 | 211 | 0 |
| 96 kHz, stereo, 24-bit | 576,000 | 1,101 | 434 | 0 |
| 96 kHz, stereo, 32-bit | 768,000 | 1,522 | 1,104 | 349 |

The worker now sleeps only when no frames were submitted, not after every
block. This improves high-rate playback, but does not remove the source-I/O
limit. At 10 MHz, the adaptive 96 kHz stereo 24/32-bit cases spent
5.312/7.092 seconds reading a five-second file; at 20 MHz these fell to
3.793/5.086 seconds. Lock waits were only about 1.6 ms per file, so shared-SD
lock contention was not the dominant limit in these tests. Raising SPI speed
helped, but SD/SPI/cache/interrupt overhead matters as well as nominal bus
bandwidth; 20 MHz did not reliably sustain 96 kHz stereo 32-bit.
`READ_US`, `WAIT_US`, `MAX_READ_US` and `SPI_MHZ` expose these measurements.

For normal use, **44.1/48 kHz 16-bit PCM** is the conservative recommendation.
48 kHz stereo 32-bit was the highest tested zero-underrun input bandwidth at
10 MHz (3.072 Mbit/s); 96 kHz stereo 24-bit passed at 20 MHz (4.608 Mbit/s).
These are measured five-second cases on this card, not universal guarantees,
acoustic-quality measurements or bit-perfect 24/32-bit output.

Diagnostic commands (queued; poll GET_AUDIO for completion):

```text
AUDIO_FIXTURE 48000 16 2 5 /BNTEST1234abcd
START_AUDIO 0 /BNTEST1234abcd/TEST.WAV
STOP_AUDIO
AUDIO_CLEAN /BNTEST1234abcd
AUDIO_SPI 20
AUDIO_SPI 10
```

Fixture arguments are rate, input bits, channels, seconds (1–10), and a fresh
folder `/BNTEST` plus eight lowercase hexadecimal digits. Existing folders
are rejected. Cleanup verifies the exact ownership marker and refuses folders
containing unrelated entries. SPI diagnostics stop playback and accept
10/20 MHz. They fail BUSY while another
diagnostic file/directory is open. Reboot restores 10 MHz. Do not change SPI
while writing settings; use 10 MHz for fixture creation/deletion and restore it
after read-only comparisons.

## Sleep Timer

The Sleep timers menu provides three saved configurations (`SLT1`-`SLT3`),
then Stop sleep, Time left and Back. Select a slot to edit Hold minutes, Outputs, Animation,
Fade Out, RGBW color, brightness, sound and volume.
Outputs can be Lights only, Audio only or Both. Longer labels and animation
names scroll in full, with 900-ms pauses at the first and last four letters
and slower movement between them. Numeric editors show only the value, such as `1`, `5`,
`10`, without repeatedly scrolling the setting name. Alarm and standalone
light editors use the same full-label/value-only presentation.
Sound uses the same root-directory WAV picker as alarms; additional paths
can be selected through serial. Every valid adjustment immediately saves to the
selected slot. Start sleep starts that slot's **saved** configuration,
including when restarting an already-running timer.
Stop cancels it immediately, without waiting for the fade-out.

Hold (0-180 minutes) includes the initial fixed 30-second rise of both enabled
outputs; they remain at peak for the rest of Hold. Fade Out controls both
falling ramps. Total duration is exactly Hold + Fade Out, not an extra 30 seconds.
With Hold zero the rise is skipped and fade-out begins immediately at peak.
With both Hold and Fade Out zero the timer completes immediately without starting
audio. Alarm and standalone light timing are unchanged.
During the final Fade Out period, light fades twice as fast: it reaches black
at the period's midpoint and stays black while audio continues fading until
timer expiry. With a five-minute Fade Out, light is off 2:30 into that period
and audio is off at 5:00. Zero stops both immediately at expiry.
Fade choices are 0, 1, 5, 10, 15, 20, 30, 45 and 60 minutes.
Default slots use Beach, Fire and Nebula
respectively: 30-minute Hold, brightness 128 and five-minute Fade Out.
Audio is disabled until
a file and audio output are selected.

While sleep runs the screen is forced dark, even outside configured dark
hours. Interaction wakes it for the saved wake interval (default 30 seconds); the first
input wakes a blank clock without navigating. During the wake window the
clock and remaining time alternate every two seconds. Remaining is MM:SS
below one hour or HH:MM above it, with a steady colon and final-digit decimal
point; the real clock has a blinking colon. Idle menus return to the blank
clock after the wake window. A selected WAV fault shows `AErr` in the timer
phase; valid light continues and GET_SLEEP/GET_AUDIO expose the error.
Physical taps also wake the screen and halve both outputs; front-button wiring
is still pending.

Natural expiry scrolls `Goodnight` once over four seconds only outside
configured dark hours, then holds the clock off until interaction or the
**next upcoming noon**: finishing at 00:30 releases at noon that day; finishing
at 13:00 releases at noon the next day. Manual Stop has no completion greeting
or post-sleep hold. Alarms retain display priority. With an invalid RTC the
hold waits for a valid clock to establish noon. The post-sleep hold itself
is runtime-only and clears on reboot.

Sleep owns both output channels, switching off unselected outputs and
preempting manual playback/lighting. WAV playback loops and its time envelope
is processed entirely on core 1. Expiry switches both outputs off. A new alarm
cancels sleep immediately; sleep cannot start during an alarm or automatically
resume after that alarm ends. Manual lighting/audio starts return BUSY while
sleep owns the outputs; live
volume changes remain available within the 50% cap. The separate Live lights
menu starts standalone animations, not sleep presets. Its ownership conflicts
now scroll `Timer active`, rather than the generic `LErr`; genuine light
failures show `Light error`. Edit a saved timer under Sleep timers / SLT1 /
Animation; changes save automatically before Start sleep. STOP_AUDIO silences only its audio,
including an as-yet-pending start; the light and countdown continue.

The three configurations persist in CRC-checked alternating 512-byte
`BNSL0.DAT`/`BNSL1.DAT` SD records, with sync/close/readback verification.
Version-2 records store Hold. Version-1 records derive Hold = Duration minus
old Fade In minus Fade Out. The current timing rule includes the 30-second rise
within that stored Hold; existing numeric Hold/Fade values are not rewritten.
`GET_SLEEP` reports Hold, Total Seconds and
Fade In Seconds. Set Hold with `SET_SLEEP [slot] HOLD minutes`; legacy
`DURATION` setters explicitly report `USE_HOLD`, and Fade In setters report
`FIXED_FADE_IN` rather than silently ignoring a requested change.
Fade In Seconds is zero when Hold is zero, otherwise 30.
Blank storage uses defaults; corrupt, unsupported or unavailable storage is
reported explicitly and not overwritten. Failed saves disable further writes
until reload/restart. An older valid record can recover an interrupted save.
As with alarm/settings writes, a reported readback failure can still leave a
valid newer record that loads on restart. Configurations return after restart,
but sleep playback never resumes. Saving/editing while running does not alter
the immutable active snapshot. Standalone light presets and physical
potentiometer integration remain unfinished.

```text
SET_SLEEP FADE_OUT 0
SET_SLEEP HOLD 10
SET_SLEEP EFFECT BEACH
SET_SLEEP SOUND /Ooblets_32.wav
SET_SLEEP OUTPUT BOTH
SET_SLEEP VOLUME 5
SET_SLEEP FADE_OUT 5
SAVE_SLEEP 1
START_SLEEP 1
GET_SLEEP 1
GET_AUDIO
STOP_SLEEP
```

### Inclusive sleep Hold and menu verification (2026-11-22)

The uploaded firmware includes the 30-second sleep rise within Hold.
Slot 1 was saved with Hold 10, Fade Out 10, Nebula and `/WhaleSeaSleep.wav`,
both outputs enabled, brightness 128 and its unchanged 5% volume.
`GET_SLEEP 1` reports `TOTAL_SECONDS 1200`; the complete saved configuration
survived firmware reboot. Other saved slots, alarms and display settings were
unchanged. Host tests check the exact 20-minute deadline, peak through minute
10, lights black at minute 15, audio silence at minute 20, zero Hold/Fade
boundaries and wraparound. Strict and ASan/UBSan seven-executable suites pass.
The firmware build uses 155,808 program bytes and 23,604 static RAM bytes.

A short muted native run opened the whale WAV and advanced its PCM frames
with zero underruns. Encoder diagnostics verified the full scrolling Nebula
selection in the sleep editor without a light action, then reproduced a
standalone light ownership conflict and verified scrolling `Timer active`
while sleep continued. The check was stopped and slot 1 reloaded; final
playback and LEDs are off, and the 50% volume cap remains in force.
The complete 20-minute run was not repeated on hardware for this revision.

### Earlier separate-rise Hold verification (2026-11-22)

The following captures predate the inclusive sleep Hold change above;
their extra 30-second sleep rise is historical, not the current timing rule.

Strict host and ASan/UBSan suites cover the 30 + 10 + 10 alarm example,
the fixed sleep rise, zero holds/fades, longest supported timers, legacy
Hold migration and active alarm recovery. The uploaded build uses 151,496
bytes of program space and 23,508 bytes of static RAM.

At 2% audio, the device verified an alarm with one-minute rise, hold and fall:
audio waited through the first 30 seconds, both held at peak from 60 to 120,
audio stopped at 150 while lights continued to 180 seconds. A sleep test
rose for 30 seconds, held through 90, reached black at 120 while audio
continued until 150. All 17 samples had zero underruns. The native Hold label,
on-SD version migration and original saved settings were verified.

The requested Ocean test was then started with Beach, `/UnrealOcean.wav`,
5% volume, a 10-minute Hold and five-minute Fade Out. The 30-second rise
and moving peak animation were verified; total duration is 15:30.

SET_SLEEP accepts DURATION, OUTPUT (LIGHT|AUDIO|BOTH), EFFECT, COLOR (four
0–255 RGBW values), BRIGHTNESS (0–255), FADE_IN, FADE_OUT,
SOUND (NONE or an absolute path), VOLUME (0–10), AUDIO_FADE_IN and AUDIO_FADE_OUT.
The AUDIO_FADE names are compatibility aliases for the shared FADE settings,
not separate audio timings. `SET_SLEEP n FIELD value` saves a field directly
to slot 1–3; `SAVE_SLEEP n` saves the RAM draft, `LOAD_SLEEP n` loads a saved
slot into the draft, and `START_SLEEP n` starts a saved slot.
Commands without a slot keep the RAM-draft workflow.
Select a sound before enabling AUDIO/BOTH. Invalid combinations return
`ERR SLEEP INVALID_VALUE` and leave the previous configuration unchanged.
START_SLEEP/STOP_SLEEP return their command acknowledgements;
GET_SLEEP reports SLOT and ACTIVE_SLOT (1–3, or 0 for the RAM draft), state,
remaining seconds, elapsed milliseconds, active
channel selection, next-start settings and current audio state/error.
GET_AUDIO's SLEEP/ALARM flags identify the actual audio owner.

### Sleep verification (2026-11-22)

Seven host executables, address/undefined-behavior sanitizers and the Feather
build passed (148,376 bytes program; 23,360 bytes static RAM).
The uploaded firmware produced 220 samples across a real two-minute sleep
timer: one minute fading up and one minute fading down, with **2%** peak audio
gain. A two-second, 32 kHz mono 16-bit WAV looped through 3,843,072 submitted
frames, with zero underruns and worker/DMA on core 1. Gain reached 199/200
basis points and commanded red reached the 15/255 cap; both stopped at expiry.

Device checks also passed light-only and audio-only operation, cancellation,
manual STOP_AUDIO without restarting sound, busy guards, native encoder
sound selection/Start/Stop, and alternating `SLP`/countdown display. The
picker played the existing Ooblets WAV. A real scheduled one-shot alarm
preempted sleep's light and audio at 1% gain; dismissing it did not resume
sleep. A missing WAV reported FILE while light/countdown continued without
repeated playback attempts. Restart into the identical firmware left sleep,
audio and LEDs off. Each new effect produced twelve distinct-time frame
samples; host tests additionally verify palettes, spatial variation, fades
and bounded frame-to-frame changes.

The device run caught and fixed independent menu-action dispatch before
these final checks passed. Temporary audio was removed, all original alarm
and display configurations and SD directory names/sizes were restored, the
RTC continued advancing without edits, and SPI stayed at 10 MHz. Audio and
LEDs were left off. These are commanded-frame/I2S checks, not optical or
acoustic measurements or a new listening assessment.

### Saved sleep/shared fade verification (2026-11-22)

This records the preceding equal-rate version; the timing amendment below
supersedes its ramp behavior.

Seven strict host executables and address/undefined-behavior sanitizer runs
passed, including full three-slot record roundtrips, interrupted writes,
failed readback, unsupported-version protection, alarm migration, shared
fade aliases, compact numeric editors and wrap-safe wake/noon policies.
The uploaded Feather build uses 150,896 bytes program and 23,744 bytes static RAM.

Native encoder editing, Save and Run produced a real two-minute sleep with
one shared minute fading up and one fading down. Across 133 samples, audio
reached 198/200 gain basis points at **2%** configured volume, red reached
15/255, and the 32 kHz Ooblets WAV submitted 3,843,072 frames without underruns.
Worker and observed DMA interrupts stayed on core 1. Saving a different
duration/volume mid-playback left the active two-minute/2% snapshot unchanged.
Clock/countdown alternation, interaction wake, idle-menu blanking, one-time
Goodnight and next-day noon release passed. A separate dark-hours expiry
skipped Goodnight; interaction cleared the hold, and manual cancellation
returned to the clock without a completion message.

All three saved configurations survived restarting into the identical
firmware while sleep was active; sleep/audio/LEDs did not resume.
Beach aqua, white-free cool Nebula and slow Fire produced sampled commanded
frames; host tests bound Fire/Nebula changes to four logical channel units
per 50 ms over a minute. A shared alarm fade-in reported 68/200 audio gain
basis points with matching light rise, preempted sleep, and did not resume
sleep after dismissal.

Original alarms, display settings and the advancing RTC were restored.
The earlier RAM sleep draft was retained with its light fade times now shared.
Three default saved slots use Beach/Fire/Nebula, 30 minutes, one-minute
fade-in and five-minute fade-out, light only. Original SD directory
names/sizes remain, plus the two intentional 512-byte sleep records.
SPI stayed at 10 MHz and audio/LEDs were left off.
These are software/display-text/commanded-frame checks, not new optical or
acoustic measurements or listening approval.

### Staggered timing verification (2026-11-22)

The timing amendment passed all seven host executables, address/undefined-
behavior sanitizers and the Feather build (150,936 bytes program; 23,744 bytes
static RAM). Host tests cover the exact five-minute examples, zero durations,
alarm recovery, unchanged sleep fade-in and unchanged alarm fade-out.

At **2%** audio, 66 native samples of a one-minute alarm fade showed no
streamed frames during the first 30 seconds. At 30 seconds light was 7/255
with zero audio gain; at 45 seconds light was 11/255 and audio 100/200 basis
points; both reached peak at 60 seconds. A two-minute sleep with a final
one-minute fade produced 127 samples: light was black at 90 seconds while
audio remained at 98/200 basis points, then 48/200 at 105 seconds, and stopped
at 120 seconds. Both runs had zero underruns, with playback/DMA on core 1.
Audio gain reporting includes buffered output; these are commanded-frame and
software timing checks, not acoustic/optical measurements.

Original alarms, saved sleep slots, RAM draft, display settings and SD
directory contents were restored/unchanged; the RTC continued advancing.
No persistence format or menu setting changed. Audio and LEDs were left off.

## Light Effects

Light Effects offers Sunrise, Color, Forest and Stop, followed by Fade In,
Fade Out, RGBW color, Brightness, Beach, Fire, Nebula and Back. Select an effect to
start it; edit a field by rotating and press the encoder to return to the
list. Fade times are 0, 1, 5, 10, 15, 20, 30, 45 or 60 minutes. Fade-in starts
when the effect starts; Stop fades from the current frame to black, with zero
meaning immediate. The forest frame is held while it fades out. A change to
fade-out applies at the next Stop; other edits apply when an effect is next
started. These controls are temporary and do not yet save light presets.

Sunrise uses the scrolling `Sunrise` menu label. Its first half stays in deep red,
orange and yellow, with no white or blue. During the second half white rises
alongside the warm colors; those colors recede only in the final eighth,
ending on the white channel alone. RGBW edits affect Color, not the sunrise
palette; Brightness and both fade times still apply.
Forest uses numerous randomized shadow patches moving independently up and
down with gentle side-to-side drift. Individual dark cores can cover 2x2 LEDs,
with wider soft edges that fade LEDs gradually as the shadows pass.
Motion is about 20 percent slower than the initial split-patch version.
Brightness fades in over Fade In; shadows shrink as it brightens, revealing
more light. Motion accelerates
continuously from 1x to 4x during the fade and remains fast at full intensity.
Green appears in scattered portions of shadow rims and occasional independent
spots, rather than a stripe. Green is always present. Zero Fade In starts at full brightness and speed immediately.
The revised effects were uploaded and checked through serial on 2026-11-22:
a one-minute sunrise kept white off through halfway, mixed warm channels with
white at 45 and 54 seconds, and ended on white alone. The latest one-minute
forest fade started black, showed moving 2x2 dark areas and scattered green,
and took all 64 LEDs through at least five brightness levels during sampling.
Host tests also
check bidirectional movement, accelerating motion, scattered green rims,
soft edge fades, 2x2 dark areas and continuity at fade completion.
These checks read commanded frames, not optical measurements;
visual assessment remains with the owner.

Beach moves a feathered teal/aqua tide across warm sandy RGBW light, with soft
white foam and slower rolling variations in the water. Fire uses staggered,
smoothly interpolated amber/red fluctuations with 9–16-second drift intervals
and an 18-second ember interval for calm flame light, not rapid strobing.
Nebula is primarily blue and purple, with sparse subdued pink/gold accents.
Its cool, white-free stars twinkle over roughly 28–48-second cycles and the
clouds drift slowly. These predefined
palettes ignore RGBW color edits; brightness and fades still apply.
All three are available for sleep, standalone lighting and alarms. Sleep
continues animating while fading down; standalone Stop holds its final frame
while fading, as before.

The panel uses the legacy application's `NEO_GRBW` configuration and starts
black. Colors and brightness use 0–255 logical values; the final panel-output
stage divides every channel by 16, so the maximum sent is 15/255. The owner
temporarily removed the power-budget restriction and requested less than 50%
brightness; this setting is much dimmer. It does not measure power or prove
the available USB budget. The owner
identified the panel as an [Adafruit 2871](https://www.adafruit.com/product/2871);
the legacy hardware-test sketch uses `NEO_RGBW`. The owner observed the four
channels, corner positions and a full white panel on 2026-11-22, confirming
the `NEO_GRBW` color order and tested orientation.

Settings also includes `Time Format` and `Brightness`. Rotate to change the
value; press the encoder to return to Settings. Changes are saved immediately.
Format defaults to 24-hour and brightness to hardware level 3 (range 0–15),
matching the previous display behavior. `Dark Hours` has an enable switch and
start/end hour and minute settings. It defaults off with 22:00–07:00 as the
interval; equal start and end times disable blanking. The clock blanks during
the interval and a local encoder or front-button input wakes it for 30 seconds.
The first input wakes the clock without also navigating. Other settings editors
are not yet implemented.

The display settings use two reserved files, `BNSET0.DAT` and `BNSET1.DAT`,
on the SD card. A blank card uses defaults without writing until a setting
changes; older version-1 display settings are retained when migrating on the
next successful change. Existing corrupt or unsupported settings are not
initialized over. Missing or failed storage
shows `SD Error` (press the encoder to continue); settings writes are disabled
after a failure until restart. A failed or interrupted write can have an
ambiguous outcome if the card completed the write before reporting an error;
the most recent valid record may then be loaded on restart. SD power-loss
behavior and settings restoration still require on-device verification.

Serial commands are newline-terminated. `SET_TIME YYYY-MM-DD HH:MM:SS` returns
an `OK` or `ERR` line and emits a time-change event after a successful update.
`GET_TIME` reads the RTC; `GET_SCREEN` returns the currently displayed text.
`GET_ALARM 1` through `GET_ALARM 5` report saved configuration and live state.
`SET_ALARM slot FIELD value` validates and immediately saves one field;
each local alarm adjustment automatically uses the same validated save operation.
Fields are `ENABLED 0|1`, `ONESHOT 0|1`, `TIME HH:MM`, `DAYS mask`,
`EFFECT SUNRISE|SOLID|FOREST|BEACH|FIRE|NEBULA`, `COLOR r g b w`, `BRIGHTNESS 0..255`,
`FADE_IN minutes`, `FADE_OUT minutes`, `RUN minutes`,
`SOUND NONE|/path.wav`, `VOLUME 0..10`, `AUDIO_FADE_IN minutes` and
`AUDIO_FADE_OUT minutes`. Sound paths may contain spaces and appear as the
last field of GET_ALARM. Audio fade choices match light fades.
The day mask uses Sunday bit 0 through Saturday bit 6; Monday+Friday is 34,
and at least one day must be selected.
`START_ALARM slot` starts a saved test; `SNOOZE_ALARM`, `DISMISS_ALARM` and
`HALF_ALARM` act on the current occurrence. Alarm starts emit timestamped
events. Manual LED/effect commands report `BUSY` while an alarm owns the panel.
`GET_DISPLAY` reports the current display format, brightness and `WAKE_SECONDS`.
`SET_DISPLAY WAKE_SECONDS 30` sets the interaction/tap wake interval from 1 to
300 seconds. The local setting is Settings / Dark / Wake, with a value-only
seconds editor. The default remains 30 seconds. BNSA version-3 records preserve
this setting; version-1/2 records retain their existing settings and default
to 30 seconds, upgrading on their next save.

The onboard LIS3DH uses coherent raw X/Y/Z samples at a 4g range, removing a
slowly tracked gravity baseline and detecting the combined vector magnitude
of changes across all three axes. It samples at up to 200 Hz and uses a
120-milli-g default threshold, quiet rearming and a 400-ms ringing cooldown.
This was raised from 50, then 80 milli-g after table bumps were reported as too sensitive.
The first second of raw samples calibrates gravity without accepting taps.
A median-of-three disturbance filter rejects isolated sensor spikes.
This detects acceleration disturbances, not absolute position displacement.
The first hardware click-based implementation required too much force;
the user requested this more sensitive combined-axis approach.
Accelerometer initialization/configuration/read failures are logged and disable
physical taps rather than generating synthetic events.
Each accepted tap wakes the dark or sleep display. During an alarm or sleep
timer the first tap also halves both ongoing output envelopes once,
without restarting their fades, editing presets or activating snooze.
These tap reductions are temporary: all expire when the interaction/tap
screen-wake interval ends (30 seconds by default). Further taps only restart
that interval; they do not halve again. Encoder interaction also extends it
without another halving. Expiry restores the current scheduled envelope levels, not the
levels captured before the tap. Restoration does not restart audio or fades.
The separate alarm Dim half / `HALF_ALARM` action remains persistent for the
current run and is not undone by tap expiry.
The next run starts at the configured levels. Snooze remains a front-button
action; that button is still awaiting soldering.
`TAP` injects one diagnostic tap through the same behavior handler.
`GET_TAP` reports sensor availability/error, physical accepted-tap count,
last event time, filtered current disturbance (`DELTA_MG`), raw peak
(`PEAK_MG`), filtered peak (`FILTERED_PEAK_MG`), threshold and sample count.
`SET_TAP_THRESHOLD 120` tunes sensitivity live from 10 to 500 milli-g: lower
is more sensitive. This diagnostic calibration resets to 120 after reboot;
the display wake duration remains a saved user setting.
Injected taps bypass the
physical filter and do not increment its count.

Non-stacking tap dimming was uploaded and verified on 2026-11-22 at the new
120-milli-g default. With a three-second wake setting, diagnostic taps kept
sleep audio at 100 basis points and LED white at 4 after the first halving
from 200/8; repeated taps did not submit another audio-reduction request.
A second tap extended the dim period past the original expiry. Encoder
interaction also extended it; final expiry restored 200/8 and blanked the
screen without restarting playback. A fresh tap after expiry began a new
half-level interval and restored normally. There were zero underruns.
All seven strict host test executables and the firmware build passed.
Latest presets and the 30-second wake setting were restored; playback is off.

Earlier cumulative temporary tap restoration was verified on 2026-11-22
(superseded by the non-stacking behavior above).
With a three-second wake setting, diagnostic taps reduced sleep gain
200 -> 100 -> 50 basis points and LED white 8 -> 4 -> 2. Encoder interaction
extended both the wake and attenuation; after expiry the screen blanked and
gain/white returned to 200/8 while frames and elapsed time continued, with
zero underruns. The device reported the new 80-milli-g default after upload.
Host tests cover alarm restoration during fade-out, persistent manual Dim
half, capped repeated taps, and retrying audio restoration after BUSY.
All seven strict and ASan/UBSan test executables and the firmware build pass.
Latest saved presets and wake duration were preserved, including the user's
updated slot-1 volume of 50%; playback was stopped after verification.
Sensitivity still depends on the physical mounting/surface and can be tuned
with the diagnostic command.

The earlier 50-milli-g filtered firmware was verified on the Feather on 2026-11-22.
Two gentle physical taps reduced sleep audio gain from 200 to 100 to 50 basis
points and LED white-channel levels from 8 to 4 to 2, while the timer continued
with zero audio underruns. The user confirmed both taps woke the screen.
No accepted taps occurred during the preceding five-minute monitoring interval.
Diagnostic tests verified three-second wake expiry, wake-duration persistence
across firmware reboot, and alarm tap handling without snoozing or restarting.
Strict host and ASan/UBSan suites passed. Original display/dark settings and the
Ocean sleep draft were restored; all five alarms and three saved sleep slots
were unchanged, and test playback was stopped.

`SET_DISPLAY FORMAT 12` or `SET_DISPLAY FORMAT 24` changes the format;
`SET_DISPLAY BRIGHTNESS 0` through `SET_DISPLAY BRIGHTNESS 15` sets brightness.
Both menu and serial changes use the same validated, immediate-save operation.
`GET_DARK` reports the dark-hours configuration and whether the clock display
is currently blanked. `SET_DARK ENABLED 0|1`, `SET_DARK START HH:MM` and
`SET_DARK END HH:MM` edit it. `ENCODER LEFT`, `ENCODER RIGHT` and
`ENCODER CLICK` simulate the corresponding local encoder inputs.
`START_LIGHT effect red green blue white brightness fade_in fade_out`
starts an effect; `effect` is `SUNRISE`, `SOLID`, `FOREST`, `BEACH`, `FIRE`
or `NEBULA`, color and
brightness are 0–255, fade values are the selectable minutes listed above.
`STOP_LIGHT` starts the
configured fade-out. `GET_LIGHT` reports the run state and active effect
configuration. Light settings shared by menu and serial are not persisted.
`SET_LED row column red green blue white` sends one pixel (row and column
0–7, with (0,0) at bottom left); each requested component is 0–255 and is
divided by 16 at the final output stage. Other pixels remain unchanged.
`SET_LED` cancels any running effect. `CLEAR_LEDS` cancels it and immediately
turns the panel off. `GET_LEDS`
returns `OK LEDS ` followed by 64 eight-character hexadecimal RGBW values in
row-major order, starting at the bottom-left (0,0) and proceeding left to right
and then bottom to top. This reports the scaled values sent to the panel, not
sensor readings.
`BOOTLOADER` replies `OK BOOTLOADER` and then reboots into USB programming
mode, stopping normal clock operation and switching off external power. The
reply is sent before reboot, but cannot be guaranteed if USB disconnects. The
command requires serial diagnostics to have been enabled at startup. The
RP2040 USB stack also supports a 1200-baud/DTR reset into the bootloader, even
when application diagnostics are disabled. Both paths were verified on the
Feather on 2026-11-22, including the 1200-baud reset after a start without a
serial client. Reboot only when interruption is safe; the physical boot
button remains the recovery method if USB is unresponsive.
`SD_LIST` lists the root directory read-only; `SD_LIST /path` lists another
directory. A successful response is `OK SD_LIST BEGIN`, then `ENTRY D|F size
name` lines, then `OK SD_LIST END`. Errors use `ERR SD_LIST ...`. Names and
paths can contain spaces. `SD_READ /path` returns file bytes as hexadecimal
`DATA` lines between `OK SD_READ BEGIN` and `OK SD_READ END`. Serial diagnostics
are enabled only when USB CDC DTR is
asserted during the bounded 1.5-second startup window; open the host serial
client before powering or resetting the Feather. Saved light presets are
not implemented yet.

Example:

```text
SET_TIME 2026-11-21 15:19:45
OK SET_TIME 2026-11-21 15:19:45
EVENT TIME_SET 2026-11-21 15:19:45
```

Command responses are prioritized over queued event logs. If a slow/disconnected
client causes event logs to be dropped, the next available event slot reports
the dropped count.

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
by these commands. An earlier peripheral diagnostic was uploaded separately;
this does not verify the current rewrite or all physical hardware behavior.

The project uses `main`, `dev`, and feature branches. The menu rewrite is still
in progress on a feature branch; its current firmware is uploaded, but
device-level behavior and the remaining peripherals still need verification.
