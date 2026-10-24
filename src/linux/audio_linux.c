/*
 * ALSA audio output (see audio_linux.h), after orange386's audio_linux.c.
 */
#include "audio_linux.h"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_PERIOD      256        /* frames per write, ~5.8 ms at 44.1 kHz */
#define AUDIO_LATENCY_US  40000u

static snd_pcm_t *pcm;
static pthread_t audio_thread;
static atomic_bool audio_running;
static bool audio_started;
static audio_fill_t audio_fill;

static void *audio_main(void *arg)
{
	(void)arg;
	int16_t buf[AUDIO_PERIOD * 2];
	while (atomic_load(&audio_running)) {
		audio_fill(buf, AUDIO_PERIOD);
		snd_pcm_sframes_t n = snd_pcm_writei(pcm, buf, AUDIO_PERIOD);
		if (n < 0) {
			n = snd_pcm_recover(pcm, (int)n, 1);
			if (n < 0) {
				fprintf(stderr, "audio: %s\n", snd_strerror((int)n));
				break;
			}
		}
	}
	return NULL;
}

/*
 * Sound output device, first match:
 *   AUDIODEV in the environment (as SDL programs use it),
 *   the HDMI card, which carries the picture as well,
 *   ALSA "default".
 */
static const char *audio_device_name(void)
{
	const char *dev = getenv("AUDIODEV");
	if (dev && *dev)
		return dev;
	if (snd_card_get_index("HDMI") >= 0)
		return "plughw:CARD=HDMI,DEV=0";
	return "default";
}

bool audio_init(unsigned rate, audio_fill_t fill)
{
	if (audio_started)
		return true;
	const char *dev = audio_device_name();
	int err = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		fprintf(stderr, "audio: cannot open %s: %s (sound disabled)\n",
			dev, snd_strerror(err));
		pcm = NULL;
		return false;
	}
	err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED,
				 2, rate, 1, AUDIO_LATENCY_US);
	if (err < 0) {
		fprintf(stderr, "audio: %s: %s (sound disabled)\n", dev, snd_strerror(err));
		snd_pcm_close(pcm);
		pcm = NULL;
		return false;
	}
	audio_fill = fill;
	atomic_store(&audio_running, true);
	err = pthread_create(&audio_thread, NULL, audio_main, NULL);
	if (err) {
		fprintf(stderr, "audio: cannot start thread: %s\n", strerror(err));
		atomic_store(&audio_running, false);
		snd_pcm_close(pcm);
		pcm = NULL;
		return false;
	}
	audio_started = true;
	printf("audio: %s, %u Hz stereo\n", dev, rate);
	return true;
}

void audio_shutdown(void)
{
	if (!audio_started)
		return;
	atomic_store(&audio_running, false);
	pthread_join(audio_thread, NULL);
	snd_pcm_drop(pcm);
	snd_pcm_close(pcm);
	pcm = NULL;
	audio_started = false;
}
