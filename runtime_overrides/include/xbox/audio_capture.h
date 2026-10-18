#ifndef XBOX_AUDIO_CAPTURE_H
#define XBOX_AUDIO_CAPTURE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum AudioCaptureStatus
{
    AUDIO_CAPTURE_IDLE,
    AUDIO_CAPTURE_REQUESTED,
    AUDIO_CAPTURE_RECORDING,
    AUDIO_CAPTURE_COMPLETE,
    AUDIO_CAPTURE_FAILED
} AudioCaptureStatus;

/* Debug flight recorder: any host thread may request or observe capture.
 * Repeated requests during a pending/active capture are ignored. The APU thread
 * owns files and history; readers receive only an atomic progress snapshot.
 * remaining_ms measures mixed audio still to record, not a wall-clock estimate.
 * Completed/failed status persists until the next request. */
void apu_debug_audio_request(void);
AudioCaptureStatus apu_debug_audio_status(unsigned *remaining_ms);

#ifdef __cplusplus
}
#endif
#endif
