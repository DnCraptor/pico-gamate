#ifndef ORANGE_GAMATE_AUDIO_LINUX_H
#define ORANGE_GAMATE_AUDIO_LINUX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ALSA playback, the Linux counterpart of the RP2350 I2S output: a playback
 * thread asks the emulator for interleaved stereo S16 frames (L, R) and
 * writes them to the PCM device.  The device is $AUDIODEV, else the ALSA
 * card "HDMI" (plughw:CARD=HDMI,DEV=0), else "default".
 */

typedef void (*audio_fill_t)(int16_t *frames, unsigned count);

/* Start playback at rate Hz; false (sound disabled) if no device opens. */
bool audio_init(unsigned rate, audio_fill_t fill);

/* Stop the playback thread and close the PCM device. */
void audio_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_GAMATE_AUDIO_LINUX_H */
