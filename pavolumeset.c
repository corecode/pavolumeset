#include <stdio.h>
#include <err.h>
#include <string.h>
#include <pulse/pulseaudio.h>


struct sinkdata {
	int		success;
	uint32_t	index;
	pa_channel_map	channel_map;
	pa_cvolume	volume;
};

static void
pactx_cb(pa_context *ctx, void *data)
{
	int *readyp = data;

	pa_context_state_t state = pa_context_get_state(ctx);
	switch (state) {
	case PA_CONTEXT_READY:
		*readyp = 1;
		break;

	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		*readyp = -1;
		break;

	default:
		break;
	}
}

static void
pasink_cb(pa_context *ctx, const pa_sink_info *i, int eol, void *data)
{
	struct sinkdata *sd = data;

	if (eol == 0 && i) {
		sd->index = i->index;
		sd->channel_map = i->channel_map;
		sd->volume = i->volume;
		sd->success = 1;
	}
}

static void
parse_and_set_volume(pa_cvolume *vol, const pa_channel_map *map, int argc, char **argv)
{
	for (int i = 0; i < argc; ++i) {
		char channame[100];
                char *volstr, *endp;
                double voldb;

		strncpy(channame, argv[i], sizeof(channame));
		channame[sizeof(channame) - 1] = 0;

		if (!strtok(channame, "=:") ||
		    !(volstr = strtok(NULL, "=:")) ||
		    ((voldb = strtod(volstr, &endp)), *endp != 0)) {
			fprintf(stderr, "cannot parse volume setting `%s'\n", argv[i]);
			continue;
		}

		pa_channel_position_t pos = pa_channel_position_from_string(channame);
		if (pos == PA_CHANNEL_POSITION_INVALID) {
			fprintf(stderr, "invalid channel name `%s'\n", channame);
			continue;
		}

		if (!pa_cvolume_set_position(vol, map, pos, pa_sw_volume_from_dB(voldb))) {
			fprintf(stderr, "channel `%s' (%d) does not exist\n", channame, pos);
			continue;
		}

		printf("set channel `%s' to %lfdB\n",
		       pa_channel_position_to_pretty_string(pos),
		       pa_sw_volume_to_dB(pa_cvolume_get_position(vol, map, pos)));
	}
}

static void
success_cb(pa_context *ctx, int success, void *data)
{
	printf("success: %d\n", success);
}

int
main(int argc, char **argv)
{
	pa_mainloop *pam = pa_mainloop_new();
	pa_mainloop_api *pamapi = pa_mainloop_get_api(pam);
	pa_context *pactx = pa_context_new(pamapi, "pavolumeset");

	pa_context_connect(pactx, NULL, 0, NULL);

	int ready = 0;
	pa_context_set_state_callback(pactx, pactx_cb, &ready);

	pa_operation *paop = NULL;

	struct sinkdata sinkdata = { .success = 0 };

	enum {
		START,
		SINKINFO,
		DONE,
	} state = START;

	while (ready >= 0) {
                pa_mainloop_iterate(pam, 1, NULL);

                if (ready == 0)
			continue;

		if (paop) {
			if (pa_operation_get_state(paop) != PA_OPERATION_DONE)
				continue;
			pa_operation_unref(paop);
			paop = NULL;
		}


		switch (state) {
		case START:
			paop = pa_context_get_sink_info_by_name(pactx, argv[1], pasink_cb, &sinkdata);
			state = SINKINFO;
			break;

		case SINKINFO:
			if (!sinkdata.success) {
				errx(1, "unknown sink `%s'", argv[1]);
			}
			parse_and_set_volume(&sinkdata.volume,
					     &sinkdata.channel_map,
					     argc - 2, argv + 2);
			paop = pa_context_set_sink_volume_by_index(pactx,
								   sinkdata.index,
								   &sinkdata.volume,
								   success_cb, NULL);
			state = DONE;
			break;

		case DONE:
			goto out;
		}
	}
out:

	return (0);
}
