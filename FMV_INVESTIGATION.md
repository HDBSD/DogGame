# FMV playback investigation

The reported failure is smooth playback for the first few seconds followed by
stuttering, especially in the new-game movie. Playback continues; a decoder
failure, crash or stopped output has not been established.

## Evidence and presentation change (18 October 2026)

The supported title's new-game movie is `cin1.xmv` (identified by
`title_guest_movie_is_initial_cinematic`). The maintained playback wrapper
`title_guest_play_menu_movie` calls the translated guest routine `sub_000A3330`.
The original translated XMV decoder remains in use. These references come from
existing source/instruction translations, not a fresh Ghidra recovery.

Inspection of `fb_present.c:fb_thread` established that conversion and drawing
were followed by an unconditional `Sleep(16)`. Consequently every iteration
took its drawing time plus the sleep, rather than fitting that work into a
refresh interval. The window now waits against a QPC deadline at 60 Hz, carries
fractional ticks, and resets its deadline after a substantial stall. A Windows
high-resolution waitable timer has ordinary-timer and Sleep fallbacks. The clock
and timer belong exclusively to the window thread and are released there. This
does not change guest decoding, field counters, simulation timing or movie data.

`tests/Check-WindowRefresh.py` exercises the actual pacing helpers with 4 ms
of simulated drawing per iteration, deadline recovery after an 80 ms stall,
and missing-timer fallback. Thirty refreshes took approximately 500 ms.
The display-resolution fixture separately checks composition and input while
publication is blocked.

## Private reproduction and limits

An isolated `build/fmv-probe/game/fmv/wblogo.xmv` copy of `cin1.xmv` allowed
40-second startup probes without selecting New Game or creating a save. The
original movie files were untouched. Probes used hidden windows and `--noaudio`
(muted output, with audio emulation still running). This does not reproduce the
full new-game context or establish visible/audio synchronization quality.

With `RECOMP_OVERLAY_STATS=1`, the baseline reported about 27 observed content
updates per second after its first interval; deadline pacing reported roughly
28–29. This samples changes in PVIDEO surface contents, not decoded frame
counts: partial writes and static images limit interpretation. The improvement
is modest and does not establish that the user's stutter is fixed.

The existing opt-in `RECOMP_FRAMECOUNTER_CLOCK=1` path was also tested, alone,
with a precise clock experiment, and together with deadline presentation.
None established an improvement; the precise field-clock experiment was
removed and the default field-clock policy remains unchanged. Translated
`sub_000FF010` and `loc_000FD227` refer to the registered counter as part of
display/field handling, but its entire timing contract has not been recovered.

File reads continued beyond 13 MB during a baseline probe, which rules out a
complete initial-buffer refill hang in that run. It does not rule out uneven
streaming. A limited thread-instruction sample mostly found waits, without
sufficient attribution to establish decoder saturation or a synchronization
cause. Decoder changes would currently be speculative.

Remaining work is to correlate guest decode/publication boundaries, consumed
movie timestamps and visible presentation intervals during an actual affected
new-game playback. No complete FMV fix or full playback validation is claimed.
