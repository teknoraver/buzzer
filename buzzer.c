#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <math.h>

/* from linux/include/linux/timex.h */
#define PIT_TICK_RATE		1193182ul

#define CONTROL_WORD_REG	0x43
#define COUNTER2		0x42
#define SPEAKER_PORT		0x61

union freq {
	uint16_t freq;
	struct {
		uint8_t lob;
		uint8_t hib;
	};
};

static int port = -1;

static void play(double f, int dur)
{
	uint8_t p61;

	if (f && f > 19 && f < 20000) {
		union freq freq = { .freq = PIT_TICK_RATE / f };

		/* set buzzer
		 * 0xB6
		 * 1 0          Counter 2
		 * 1 1          2xRead/2xWrite bits 0..7 then 8..15 of counter value
		 * 0 1 1        Mode 3: Square Wave
		 * 0            Counter is a 16 bit binary counter (0..65535)
		 */
		if (!pwrite(port, "\xB6", 1, CONTROL_WORD_REG))
			return;

		/* select desired HZ with two writes in counter 2, port 42h */
		if (!pwrite(port, &freq.lob, 1, COUNTER2))
			return;

		if (!pwrite(port, &freq.hib, 1, COUNTER2))
			return;

		/* start beep
		 * set bit 0-1 (0: SPEAKER DATA; 1: OUT2) of GATE2 (port 61h)
		 */
		if (!pread(port, &p61, 1, SPEAKER_PORT))
			return;

		if ((p61 & 3) != 3) {
			p61 |= 3;
			if (!pwrite(port, &p61, 1, SPEAKER_PORT))
				return;
		}

	} else {
		/* stop beep
		 * clear bit 0-1 of port 61h
		 */
		if (!pread(port, &p61, 1, SPEAKER_PORT))
			return;

		if (p61 & 3) {
			p61 &= 0xFC;
			if (!pwrite(port, &p61, 1, SPEAKER_PORT))
				return;
		}
	}
	usleep(dur * 1000);
}

#define STARTS(s1, s2) !strncmp(s1, s2, sizeof(s2) - 1)

static double note2freq(const char *note, int octave)
{
	double freq = 440.0;
	int notenum;

	/* parse the note */
	if (STARTS(note, "Do") || STARTS(note, "C"))
		notenum = -9;
	else if (STARTS(note, "Re") || STARTS(note, "D"))
		notenum = -7;
	else if (STARTS(note, "Mi") || STARTS(note, "E"))
		notenum = -5;
	else if (STARTS(note, "Fa") || STARTS(note, "F"))
		notenum = -4;
	else if (STARTS(note, "Sol") || STARTS(note, "G"))
		notenum = -2;
	else if (STARTS(note, "La") || STARTS(note, "A"))
		notenum = 0;
	else if (STARTS(note, "Si") || STARTS(note, "B"))
		notenum = 2;
	else
		return 0;

	/* handle the semitone trailer, if any */
	switch (note[strlen(note) - 1]) {
	case '#':
	case 'd': /* diesis */
	case 's': /* sharp */
		notenum++;
		break;
	case 'b': /* bemolle */
	case 'f': /* flat */
		notenum--;
		break;
	}

	freq *= pow(2, (octave - 4) + notenum / 12.0);

	return freq;
}

static void reset(void)
{
	play(0, 0);
	exit(0);
}

int main(int argc, char *argv[])
{
	char *line = NULL;
	int verbose = 0;
	size_t n = 0;
	char c;

	port = open("/dev/port", O_RDWR);
	if (port == -1) {
		perror("open");
		exit(1);
	}

	while ((c = getopt(argc, argv, "vh")) != -1)
		switch (c) {
		case 'v':
			verbose = 1;
			break;
		case 'h':
			fprintf(stderr, "usage: %s [-v] <notes_file\n", *argv);
			/* fall through */
		default:
			return 0;
		}

	/* stop the buzzer if exiting / crashing */
	atexit(reset);
	signal(SIGINT, (__sighandler_t) reset);
	signal(SIGQUIT, (__sighandler_t) reset);
	signal(SIGSEGV, (__sighandler_t) reset);
	signal(SIGTERM, (__sighandler_t) reset);

	while (getline(&line, &n, stdin) > 0) {
		char *ptr = strchr(line, '\n');
		int octave = 0;
		double freq;
		int dur;

		/* skip empty lines or comments */
		if (ptr == line || *line == '#')
			continue;

		/* truncate the string */
		*ptr = 0;

		/* get the duration */
		ptr = strchr(line, '\t');
		dur = atoi(ptr + 1);
		*ptr-- = 0;

		/* if line starts with @ it's a frequency */
		if (*line == '@') {
			freq = atoi(line + 1);
		} else {
			dur *= 100;
			/* get the octave */
			if (*line != 'P') {
				octave = *ptr - '0';

				/* strip the octave from the note */
				*ptr-- = 0;
			}

			freq = note2freq(line, octave);
		}

		if (verbose)
			printf("note: %3s, octave: %d, dur: %3d, freq: %4.2f\n",
			       line, octave, dur, freq);
		play(freq, dur);

		free(line);
		line = NULL;
		n = 0;
	}

	return 0;
}
