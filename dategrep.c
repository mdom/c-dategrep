/*
    Copyright (C) 2016 Mario Domgoergen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#define _GNU_SOURCE		// to get getline
#define _BSD_SOURCE		// to get gmtoff
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include "approxidate.h"
#include "config.h"

extern char *optarg;
extern int optind, opterr, optopt;
char const *program_name;

typedef struct {
    FILE *file;
    time_t from;
    char *name;
    pid_t pid;
} logfile;

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
char *file_extension(const char *filename);
void print_usage(void);
void print_version(void);
void parse_arguments(int argc, char *argv[], struct options *options);

char *file_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
	return "";
    return dot + 1;
}

void print_usage(void)
{
    fprintf(stderr, "Usage: %s [-f DATE] [-t DATE] [-F FORMAT]\n",
	    program_name);
}

void print_version(void)
{
    printf("dategrep version " VERSION "\n\n\
Copyright (C) Mario Domgoergen 2016\n\
License GPLv3+: GNU GPL version 3 or later.\n\
This is free software, you are free to modify and redistribute it.\n");
}

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
	// missing %z as strptime does not support it
	new_format = "%d/%b/%Y:%H:%M:%S";
    } else if (!strcmp(format, "rfc3339")) {
	// missing %z as strptime does not support it
	new_format = "%Y-%m-%dT%H:%M:%S";
    }
    return new_format;
}


void parse_arguments(int argc, char *argv[], struct options *options)
{
    int opt;
    while ((opt = getopt(argc, argv, "f:t:F:smhv")) != -1) {
	if (opt == 'f') {
	    struct timeval t;
	    if (approxidate(optarg, &t) == -1) {
		fprintf(stderr, "%s: Can't parse argument to --from.\n",
			program_name);
		exit(EXIT_FAILURE);
	    }
	    options->from = t.tv_sec;
	} else if (opt == 't') {
	    struct timeval t;
	    if (approxidate(optarg, &t) == -1) {
		fprintf(stderr, "%s: Can't parse argument to --to.\n",
			program_name);
		exit(EXIT_FAILURE);
	    }
	    options->to = t.tv_sec;
	} else if (opt == 'F') {
	    options->format = parse_format(optarg);
	} else if (opt == 's') {
	    options->skip = true;
	} else if (opt == 'm') {
	    options->multiline = true;
	} else if (opt == 'h') {
	    print_usage();
	    exit(EXIT_SUCCESS);
	} else if (opt == 'v') {
	    print_version();
	    exit(EXIT_SUCCESS);
	} else if (opt == ':' || opt == '?') {
	    print_usage();
	    exit(EXIT_FAILURE);
	}
    }
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

    char *default_format = getenv("DATEGREP_DEFAULT_FORMAT");
    if (default_format) {
	options.format = default_format;
    }

    parse_arguments(argc, argv, &options);

    if (options.from >= options.to) {
	fprintf(stderr, "%s: --from larger or equal to --to.\n",
		program_name);
	exit(EXIT_FAILURE);
    }

    if (optind < argc) {

	int no_files = argc - optind;
	logfile args[no_files];

	for (int i = 0; optind < argc; optind++, i++) {

	    char *filename = argv[optind];

	    FILE *file = fopen(filename, "r");
	    if (!file) {
		fprintf(stderr, "%s: Can't open file %s: %s.\n",
			program_name, argv[optind], strerror(errno));
		exit(EXIT_FAILURE);
	    }

	    args[i] = (logfile) {
	    /* *INDENT-OFF* */
		.file = file,
		.name = filename,
	    /* *INDENT-ON* */
	    };

	    char *extension = file_extension(args[i].name);
	    if (extension
		&& (strcmp(extension, "gz") == 0
		    || strcmp(extension, "z") == 0)) {
		int pipes[2];
		pipe(pipes);
		pid_t pid;
		if ((pid = fork()) == 0) {
		    // child
		    close(pipes[0]);
		    int file_fd = fileno(file);
		    dup2(file_fd, 0);
		    dup2(pipes[1], 1);
		    execlp("gzip", "gzip", "-c", "-d", (char *) NULL);
		} else if (pid == -1) {
		    // fork failed
		    fprintf(stderr, "%s: Cannot fork gzip: %s\n",
			    program_name, strerror(errno));
		    exit(EXIT_FAILURE);
		} else {
		    // parent
		    close(pipes[1]);
		    args[i].file = fdopen(pipes[0], "r");
		    args[i].pid = pid;
		}
	    }

	}

	for (int i = 0; i < no_files; i++) {
	    logfile current = args[i];
	    FILE *file = current.file;

	    if (current.pid) {
		process_file(file, options);
		fclose(file);
		int stat_loc = 0;
		waitpid(current.pid, &stat_loc, 0);
	    } else {

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

    bool found_from = 0;

    while ((read = getline(&line, &max_size, file)) != -1) {
	time_t date = parse_date(line, options.format);

	if (date == -1) {
	    if (options.skip) {
		continue;
	    }
	    if (options.multiline && found_from) {
		printf("%s", line);
		continue;
	    }
	    fflush(stdout);
	    fprintf(stderr, "%s: Unparsable line: %s", program_name, line);
	    exit(EXIT_FAILURE);
	}
	if (date >= options.from && date < options.to) {
	    found_from = 1;
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
    if (blksize == 0)
	blksize = 8192;

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
	    if (timestamp == -1 && !options.skip && !options.multiline) {
		fprintf(stderr, "%s: Found line without date: %s",
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
	if (timestamp == -1 && !options.skip && !options.multiline) {
	    fprintf(stderr, "%s: Found line without date: %s",
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
