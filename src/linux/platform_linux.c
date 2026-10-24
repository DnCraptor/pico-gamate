#include "platform_linux.h"
#include "audio_linux.h"
#include "input_linux.h"
#include "video_drm.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t pending = PLATFORM_RUN;
static char **saved_argv;

void platform_set_argv(char **argv)
{
	saved_argv = argv;
}

void platform_request(enum platform_request request)
{
	/* A quit outranks a reboot that is still pending. */
	if ((int)request > pending)
		pending = request;
}

/* Release everything the process holds outside itself: input grabs, the
 * CRTC (original console mode restored) and the PCM device. */
static void host_shutdown(void)
{
	input_shutdown();
	video_drm_close();
	audio_shutdown();
	fflush(NULL);
}

void platform_service(void)
{
	const int request = pending;
	if (request == PLATFORM_RUN)
		return;
	host_shutdown();
	if (request == PLATFORM_REBOOT && saved_argv) {
		/* RP2350: watchdog reset of the whole MCU.  Here: a fresh process,
		 * so every part of the emulator starts from its power-on state. */
		execv("/proc/self/exe", saved_argv);
		perror("reboot: execv");
		_exit(EXIT_FAILURE);
	}
	_exit(EXIT_SUCCESS);
}
