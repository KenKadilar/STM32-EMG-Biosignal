# lib

Portable, board-independent algorithm modules go here, each in its own subfolder with
a `library.json` or just headers + sources. PlatformIO's Library Dependency Finder
links them automatically.

Planned (ported from the thesis, see phases 3 to 4):
- `emg_dsp/`   : band-pass, rectify, envelope, feature extraction (RMS / MAV / ZC / WL)
- `classifier/`: gesture classifier (DTW or a lighter feature/threshold model)

Keeping these here (not in `src/`) makes them host-testable in `test/` and reusable.
