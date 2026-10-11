# v3.1.2

- Added per-ROM `.gmr` input recording (`R`) and playback (`P`) in the file browser while preserving the regular Demo command and in-game key mappings.
- Added frame-indexed capture of keyboard, first gamepad and second gamepad events, with live game input bypassed during replay.
- Added the `REC HH:MM:SS` recording indicator using the existing title OSD; playback has no additional overlay.
- Added periodic replay-header checkpoints and SD-card synchronization every 60 emulated frames to reduce loss after reset.
- Extended quick-save states with replay session, frame and file position information; matching quick-load truncates the abandoned branch and continues recording.
- Moved save-state files to `/.config/gamate/` and replay files to `/.config/gamate/replay/`; existing states in `/GAMATE/` require manual relocation.
- Removed repeated FatFs remounts from quick save/load to preserve open replay files and recording continuity.
- Added an optional Demo title OSD, a Demo countdown, and settings-format version 5 with migration of version-4 settings.
- Improved PSRAM/QMI startup and reset handling and fixed PIO TX-FIFO backpressure for hardware AY audio.
- Documented palette shortcuts (F6-F9), F10 browser access, replay controls, save paths and the 3.1.2 release matrix.

# v1.0.0

- Everything is working
