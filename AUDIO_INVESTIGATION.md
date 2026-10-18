# Gameplay audio investigation

The reported symptoms are intermittent pops/cracks and longer-running corruption or loss of audio during gameplay, especially E007. Monster sounds are definitely affected; overlapping sounds make the scope unclear. Loading E007 alone has not reproduced this, so a level-loading test cannot validate a fix.

## Pipeline and current evidence

`apu_vp.c` reads guest voice/stream data, resamples and filters voices into mix bins. DSP/monitor processing and the optional software mixer feed 256 stereo frames at 48 kHz. `apu_core.c:mcpx_apu_monitor_frame` groups four chunks into each 1024-frame XAudio2 submission. The backend retains three FIFO buffers and starts after two are queued; waveOut is the fallback.

The backend counts underruns and rejected full-queue submissions, which can produce gaps with otherwise valid PCM. Voice reuse already resets interpolation, ADPCM cache and filter history in `voice_reset_filters`; reuse corruption cannot be assumed merely from the symptom. Stream/block geometry, starvation/completion and persistent filter state remain candidates if the mixed PCM is corrupt. No cause of the E007 failure has been established.

## Capture the gameplay failure

Run `DogGame-AudioDiagnostic.exe --debug` in the installed game folder and play normally. Do not use `--noaudio`. When corruption is audible, press F8 before pausing/leaving, then continue for at least ten seconds. Supply the WAV, matching TXT and `debug-render.log`. The WAV retains up to ten seconds before F8 and ten afterward, before Windows playback. Recording performs file I/O on the APU thread; the pre-trigger samples help distinguish the original failure from recording overhead.

The companion report now includes output queue depth, samples played, submissions, underruns, full-queue drops, submission/start failures, last HRESULT and mixed-frame peak/saturation counts, alongside active VP/software voices. Reporting runs on the APU thread without changing samples, resetting playback or flushing buffers. XAudio2 consumes asynchronously; these observations are not atomic with guest voice state. Last HRESULT is not complete engine/device-error reporting.

Corrupt mixed PCM points toward guest voice data/decoding/filtering/mixing. Clean PCM with bad live playback points toward the output/device/backend path. Neither branch has been confirmed.

## Validation limits

Portable Clang and MSVC game builds pass with the reporting changes. No physical listening test, gameplay reproduction or E007 fix is claimed. Decoding, scheduling and guest completion behavior are unchanged by this diagnostic work.
