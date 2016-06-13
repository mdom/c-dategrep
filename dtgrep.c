#define _GNU_SOURCE		// to get getline
#define _BSD_SOURCE		// to get gmtoff
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

extern char *optarg;
extern int optind, opterr, optopt;
char const *program_name;

off_t binary_search(FILE * file, time_t from, char *timestamp_format);
void process_file(FILE * file, time_t from, time_t to,
		  char *timestamp_format);
time_t parse_date(char *string, char *format);
time_t parse_date(char *string, char *format)
{
    struct tm *parsed_dt = &(struct tm) { 0 };
    time_t now = time(NULL);
    parsed_dt = localtime(&now);
    for (char *ptr = string; *ptr; ptr++) {
	if (strptime(ptr, format, parsed_dt)) {
	    return mktime(parsed_dt);
	}
    }
    return (time_t) - 1;
}

int main(int argc, char *argv[])
{
    program_name = argv[0];
    time_t from = 0;
    time_t to = time(NULL);

    char *timestamp_format = "%b %e %H:%M:%S";

    int opt;

    while ((opt = getopt(argc, argv, "f:t:F:")) != -1) {
	if (opt == 'f') {
	    from = parse_date(optarg, "%FT%T");
	    if (from == -1) {
		fprintf(stderr, "%s: Can't parse argument to --from.\n",
			program_name);
		exit(EXIT_FAILURE);
	    }
	} else if (opt == 't') {
	    to = parse_date(optarg, "%FT%T");
	    if (to == -1) {
		fprintf(stderr, "%s: Can't parse argument to --to.\n",
			program_name);
		exit(EXIT_FAILURE);
	    }
	} else if (opt == 'F') {
	    timestamp_format = optarg;
	}
    }

    if (from >= to) {
	fprintf(stderr, "%s: --from larger or equal to --to.",
		program_name);
	exit(EXIT_FAILURE);
    }

    if (optind < argc) {
	for (; optind < argc; optind++) {

	    FILE *file = fopen(argv[optind], "r");

	    if (!file) {
		fprintf(stderr, "%s: Can't open file %s: %s.\n",
			program_name, argv[optind], strerror(errno));
		exit(EXIT_FAILURE);
	    }

	    off_t offset = binary_search(file, from, timestamp_format);

	    fseeko(file, offset, SEEK_SET);
	    process_file(file, from, to, timestamp_format);
	    fclose(file);
	}
    } else {
	process_file(stdin, from, to, timestamp_format);
    }
    exit(0);
}

void process_file(FILE * file, time_t from, time_t to,
		  char *timestamp_format)
{
    char *line = NULL;
    ssize_t read;
    size_t max_size = 0;

    while ((read = getline(&line, &max_size, file)) != -1) {
	time_t epoch = parse_date(line, timestamp_format);

	if (epoch != -1 && epoch >= from && epoch < to) {
	    size_t max_size = 25;
	    char *formatted_time = malloc(max_size);

	    struct tm *dt = localtime(&epoch);

	    strftime(formatted_time, max_size, "%FT%T%z", dt);
	    printf("%s %s", formatted_time, line);
	    free(formatted_time);
	}
    } free(line);
}


void skip_partial_line(FILE * file)
{
    int c;
    do {
	c = fgetc(file);
    } while (c != EOF && c != '\n');
    return;
}

off_t binary_search(FILE * file, time_t from, char *timestamp_format)
{
    int fd = fileno(file);
    struct stat *stats = &(struct stat) {
	0
    };
    fstat(fd, stats);
    blksize_t blksize = stats->st_blksize;

    off_t max = stats->st_size / blksize;
    off_t min = 0;
    off_t mid;

    char *line = NULL;
    ssize_t read;
    size_t max_size = 0;

    while (max - min > 1) {
	mid = (max + min) / 2;
	fseeko(file, mid, SEEK_SET);

	// skip partial line if mid != 0
	if (mid != 0) {
	    skip_partial_line(file);
	}

	if ((read = getline(&line, &max_size, file) != -1)) {
	    time_t timestamp = parse_date(line, timestamp_format);
	    if (timestamp != -1) {
		if (timestamp < from) {
		    min = mid;
		} else {
		    max = mid;
		}
	    }
	}
	free(line);
    }

    min *= blksize;
    fseeko(file, mid, SEEK_SET);
    if (min != 0) {
	skip_partial_line(file);
    }
    while (1) {
	min = ftello(file);

	read = getline(&line, &max_size, file);
	if (read == -1) {
	    break;
	}
	time_t timestamp = parse_date(line, timestamp_format);
	if (timestamp != -1 && timestamp >= from) {
	    return min;
	}
    }
    return -1;
}
