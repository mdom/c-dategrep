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
    char *filename;
    pid_t pid;
    char *line;
    time_t time;
    bool done;
    bool in_range;
} logfile;

struct options {
    time_t from;
    time_t to;
    bool multiline;
    bool skip;
    char *format;
};

struct next_log {
    time_t until;
    logfile *log;
};

off_t binary_search(FILE * file, struct options options);
void process_file(logfile * log, struct options options, time_t until);
time_t parse_date(char *string, char *format);
char *file_extension(const char *filename);
void print_usage(void);
void print_version(void);
void parse_arguments(int argc, char *argv[], struct options *options);
FILE *open_file(const char *filename);
void *safe_malloc(size_t size);
struct next_log find_next_log(logfile * logs[], int no_files, time_t);
void free_log_buffer(logfile *);
ssize_t buffered_getline(logfile *, struct options);

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

    char *default_format = getenv("DATEGREP_FORMAT");
    if (default_format) {
	options.format = default_format;
    }

    parse_arguments(argc, argv, &options);

    if (options.from >= options.to) {
	fprintf(stderr, "%s: --from larger or equal to --to.\n",
		program_name);
	exit(EXIT_FAILURE);
    }

    int no_files = argc - optind;
    if (no_files == 0) {
	no_files = 1;
	argv = safe_malloc(sizeof(char *));
	argv[0] = "-";
	optind = 0;
    }
    // logfile *logs[no_files];
    logfile *logs[10];

    for (int i = 0; i < no_files; i++) {

	char *filename = argv[optind++];

	logs[i] = safe_malloc(sizeof(logfile));
	logfile *log = logs[i];
	memset(log, 0, sizeof(logfile));
	log->filename = filename;

	char *extension = file_extension(log->filename);
	if (extension
	    && (strcmp(extension, "gz") == 0
		|| strcmp(extension, "z") == 0)) {
	    int pipes[2];
	    pipe(pipes);
	    pid_t pid;
	    if ((pid = fork()) == 0) {
		// child
		close(pipes[0]);
		FILE *input_file = open_file(log->filename);
		int file_fd = fileno(input_file);
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
		log->file = fdopen(pipes[0], "r");
		log->pid = pid;
	    }
	} else if (strcmp(log->filename, "-") == 0) {
	    log->file = stdin;
	} else {
	    log->file = open_file(log->filename);
	    off_t offset = binary_search(log->file, options);
	    if (offset == -1) {
		i--;
		no_files--;
		free(log);
	    } else {
		fseeko(log->file, offset, SEEK_SET);
	    }
	}
    }

    for (int i = 0; i < no_files; i++) {
	logfile *log = logs[i];
	process_file(log, options, options.from);

    }

    struct next_log next;

    while (next = find_next_log(logs, no_files, options.to), next.log) {
	process_file(next.log, options, next.until);
    }

    for (int i = 0; i < no_files; i++) {
	free(logs[i]);
    }
}

struct next_log find_next_log(logfile * logs[], int no_files, time_t until)
{
    time_t min = 0;
    logfile *next = NULL;
    int files_not_done = 0;

    // find logfile with lowest buffered time
    for (int i = 0; i < no_files; i++) {
	logfile *log = logs[i];
	if (!log->done) {
	    files_not_done++;
	    if (!next || log->time < min) {
		next = log;
		min = log->time;
	    }
	}
    }

    // find logfile with second lowest buffered time
    for (int i = 0; i < no_files; i++) {
	logfile *log = logs[i];
	if (log->time > min && log->time < until) {
	    until = log->time;
	}
    }
    // we can process the first file until _until_ is reached
    struct next_log next_log = {
	.log = next,
	.until = until,
    };
    return next_log;
}

ssize_t buffered_getline(logfile * log, struct options options)
{
    if (log->line) {
	return 0;
    }
    size_t max_size = 0;
    char *line = NULL;
    ssize_t read = getline(&line, &max_size, log->file);
    if (read != -1) {
	log->line = line;
	log->time = parse_date(line, options.format);
    }
    return read;
}

void free_log_buffer(logfile * log)
{
    free(log->line);
    log->line = NULL;
    log->time = -1;
}

void process_file(logfile * log, struct options options, time_t until)
{
    ssize_t read;
    while ((read = buffered_getline(log, options)) != -1) {

	// if the date can't be parsed there are three options
	// 1. -s skip line and continue
	// 2. -m print line if in range and then continue
	// 3. exit
	if (log->time == -1) {
	    if (options.skip) {
		free_log_buffer(log);
		continue;
	    }
	    if (options.multiline && log->in_range) {
		printf("%s", log->line);
		free_log_buffer(log);
		continue;
	    }
	    fflush(stdout);
	    fprintf(stderr, "%s: Unparsable line: %s", program_name,
		    log->line);
	    exit(EXIT_FAILURE);
	} else if (log->time >= options.from && log->time < options.to) {
	    log->in_range = 1;

	    if (until != -1 && log->time > until) {
		return;
	    }

	    printf("%s", log->line);
	    free_log_buffer(log);
	} else if (log->time >= options.to) {
	    break;
	} else {
	    free_log_buffer(log);
	}
    }
    log->done = 1;
    fclose(log->file);
    if (log->pid) {
	int stat_loc = 0;
	waitpid(log->pid, &stat_loc, 0);
	log->pid = -1;
    }
    free_log_buffer(log);
}

FILE *open_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
	fprintf(stderr, "%s: Can't open file %s: %s.\n",
		program_name, filename, strerror(errno));
	exit(EXIT_FAILURE);
    }
    return file;
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

void *safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) {
	fprintf(stderr, "Can't allocate memory!\n");
	exit(EXIT_FAILURE);
    }
    return ptr;
}
