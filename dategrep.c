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
#include <stdbool.h>

extern char *optarg;
extern int optind, opterr, optopt;
char const *program_name;

struct options {
    time_t from;
    time_t to;
    bool multiline;
    bool skip;
    char *format;
};

off_t binary_search(FILE * file, struct options options);
void process_file(FILE * file, struct options options);
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

char *parse_format(char *format)
{
    char *new_format = format;
    if (!strcmp(format, "rsyslog")) {
	new_format = "%b %e %H:%M:%S";
    } else if (!strcmp(format, "apache")) {
	new_format = "%d/%b/%Y:%H:%M:%S %z";
    } else if (!strcmp(format, "rfc3339")) {
	new_format = "%FT%T"; //missing %z as strptime does not support it
    }
    return new_format;
}

int main(int argc, char *argv[])
{
    program_name = argv[0];

    struct options options = {
	.format = "%b %e %H:%M:%S",
	.from = 0,
	.to = time(NULL),
	.skip = false,
	.multiline = false,
    };

    bool errflg = false;

    int opt;

    while ((opt = getopt(argc, argv, "f:t:F:")) != -1) {
	if (opt == 'f') {
	    options.from = parse_date(optarg, "%FT%T");
	    if (options.from == -1) {
		fprintf(stderr, "%s: Can't parse argument to --from.\n",
			program_name);
		exit(EXIT_FAILURE);
	    }
	} else if (opt == 't') {
	    options.to = parse_date(optarg, "%FT%T");
	    if (options.to == -1) {
		fprintf(stderr, "%s: Can't parse argument to --to.\n",
			program_name);
		exit(EXIT_FAILURE);
	    }
	} else if (opt == 'F') {
	    options.format = parse_format(optarg);
	} else if (opt == ':' || opt == '?') {
	    errflg = true;
	}
    }

    if (errflg) {
	fprintf(stderr, "Usage: %s [-f DATE] [-t DATE] [-F FORMAT]\n",
		program_name);
	exit(EXIT_FAILURE);
    }

    if (options.from >= options.to) {
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

	    off_t offset = binary_search(file, options);

	    fseeko(file, offset, SEEK_SET);
	    process_file(file, options);
	    fclose(file);
	}
    } else {
	process_file(stdin, options);
    }
}

void process_file(FILE * file, struct options options)
{
    char *line = NULL;
    ssize_t read;
    size_t max_size = 0;

    while ((read = getline(&line, &max_size, file)) != -1) {
	time_t date = parse_date(line, options.format);

	if (date == -1) {
	    fflush(stdout);
	    fprintf(stderr, "%s: Unparsable line: %s", program_name, line);
	    exit(EXIT_FAILURE);
	}
	if (date >= options.from && date < options.to) {
	    printf("%s", line);
	} else if (date >= options.to) {
	    break;
	}
    }
    free(line);
}


void skip_partial_line(FILE * file)
{
    int c;
    do {
	c = fgetc(file);
    } while (c != EOF && c != '\n');
    return;
}

off_t binary_search(FILE * file, struct options options)
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
	fseeko(file, mid * blksize, SEEK_SET);

	// skip partial line if mid != 0
	if (mid != 0) {
	    skip_partial_line(file);
	}

	if ((read = getline(&line, &max_size, file) != -1)) {
	    time_t timestamp = parse_date(line, options.format);
	    if (timestamp == -1) {
		fprintf(stderr, "%s: Found line without date: <%s>\n",
			program_name, line);
		exit(EXIT_FAILURE);
	    } else {
		if (timestamp < options.from) {
		    min = mid;
		} else {
		    max = mid;
		}
	    }
	}
    }

    min *= blksize;
    fseeko(file, min, SEEK_SET);
    if (min != 0) {
	skip_partial_line(file);
    }
    while (1) {
	min = ftello(file);

	read = getline(&line, &max_size, file);
	if (read == -1) {
	    break;
	}
	time_t timestamp = parse_date(line, options.format);
	if (timestamp == -1) {
	    fprintf(stderr, "%s: Found line without date: <%s>\n",
		    program_name, line);
	    exit(EXIT_FAILURE);
	}
	if (timestamp >= options.from) {
	    free(line);
	    return min;
	}
    }
    free(line);
    return -1;
}
