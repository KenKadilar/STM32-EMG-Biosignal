# EMG Gripper Control , Lessons Learned (2026-06-08)

A full session was spent getting one muscle to drive a gripper. Most of it was spent fighting a
symptom of one root cause. This is the record so future work doesn't repeat it.

## Goal
Single-EMG control of the 3D-printed gripper: at least open/close from one muscle, ideally a
small gesture vocabulary.

## What finally worked (the stable checkpoint)
**Raw-signal threshold TOGGLE.** The centered (pre-rectify) signal dipping below a threshold
(~ -400) toggles the gripper open<->close, one toggle per dip. It **bypasses the envelope
entirely.** CONFIRMED: 12/12 clean perfectly-alternating toggles, as fast as you can contract,
zero double-fires (`logs/dtw_log_260608-182159.csv`).

## THE ROOT CAUSE that wasted most of the session
One muscle contraction produces a **biphasic** raw signal (an up-swing then a down-swing). The
envelope used **full-wave rectification** (`abs()`), which folds the down-swing back up into a
**second positive hump , an "echo."** So every single contraction looked like **two humps** in the
envelope. Every counting/classification scheme was fed this doubled signal and miscounted.
**It was invisible until we plotted the RAW (pre-rectify) signal.** Half-wave rectify
(`max(x,0)`) removes the echo; the toggle avoids the envelope altogether.

## Approaches tried, in order, and why each failed
1. **Amplitude-coded DTW** (weak pulse = open, strong = close; classify by contraction INTENSITY).
   FAILED: amplitude is the least reproducible EMG feature , the *same* intended contraction came
   out env 98 one try and 617 another (`...141530` vs `...141651`). Fatigue / electrode drift /
   effort all move it. And at low activity the smaller template is always the closest match, so
   weak activity defaulted to "open."
2. **Timing-debounce FSM** (single vs double pulse, pulses separated by a TIMED gap). FAILED:
   needed the dip between two pulses to LAST >= ~0.12 s; real fast doubles dipped deep but BRIEF
   (~45 ms), so they merged into one (false close). Tightening the gap risked splitting jerky
   singles.
3. **Blip-counting on the envelope** (pulses separated by AMPLITUDE , env dipping below a re-arm
   line, not by a timed gap). Better idea. FAILED on the ECHO: each contraction's echo hump made
   phantom extra "blips," and the echo's zero-crossing made phantom "releases." Singles became
   doubles, doubles became singles.
4. **Band-aids stacked on #3 (all treating echo symptoms, all later removed):**
   - reprime/flush on decision -> a dc-poisoning bug (reseeding the slow DC tracker mid-contraction
     made env read garbage ~5-15 s). Patched by clearing rings only.
   - min-peak gate (a pulse must peak above X) -> rejected weak echoes but also weak real taps.
   - re-arm dwell (stay low ~0.1 s to re-arm) -> rejected brief echo dips but ALSO fast real
     doubles (their release was 1 sample, same as an echo dip). A forced, false trade-off.
   - 20 Hz high-pass -> REVERTED: the Grove sensor output is an already-enveloped LOW-frequency
     signal, so a 20 Hz HP destroyed it (movements -> tiny jitter; env -> slow sine).
   - startup fast-DC-settle + median-seed prime -> genuine small fixes (kept), not the core issue.
5. **The misdiagnosis: "electrode dropouts."** For ~6 iterations the env crashing mid-contraction
   (375->55->269) was blamed on the dying electrode and "fresh pads" was the recommended fix.
   WRONG , the raw signal was clean. The crash was env passing through the biphasic swing's
   zero-crossing; the "recovery" was the rectified echo. Found ONLY after adding a raw-signal view.

## Process lessons (Can's, in his words)
- **Get a working checkpoint at the simplest level FIRST, then build up.** Can knew the simple
  approach would work but skipped it to chase a fancier version , building toward a destination on
  a foundation that wasn't meant to support it.
- **A label can anchor you.** Once DTW was tagged "fragile," that label blocked an objective look
  and a return to basics; effort went into the half-built foundation instead of a reset.
- **When a DERIVED signal (envelope, DTW distance) looks broken, look at the RAW signal FIRST.**
  Don't theorize about hardware from a processed signal. operate.py's top "RAW signal" panel
  exists for exactly this , add it / check it before blaming the electrode.
- **One change at a time, verified.** When band-aids start stacking, you're treating symptoms ,
  stop and find the root cause.

## Current code state (post-reset)
- `emg_studio.py`: notch -> DC-tracker (fast-settle first ~1.5 s) -> **half-wave** rectify -> MA
  envelope. (Half-wave kept; the echo is gone, though the decision no longer uses the envelope.)
- `operate.py`: raw-signal **toggle** decision (`sb_thr`). 3 plots (raw / env / DTW) + CSV log.
  DTW is computed for the plot only , it drives nothing.
- Build-up path: "1 dip = close, 2 dips = open" on top of the proven raw-dip detector.
