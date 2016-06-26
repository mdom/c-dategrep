# NAME

dategrep - print lines matching a date range

# SYNOPSIS

    dategrep -f "12:00" -t "12:15" -F "%b %d %H:%M:%S" syslog
    dategrep -t "12:15" -F apache access.log
    cat syslog | dategrep -t "12:15"

# DESCRIPTION

Do you even remember how often in your life you needed to find lines in a log
file falling in a date range? And how often you build brittle regexs in grep to
match entries spanning over a hour change?

dategrep hopes to solve this problem once and for all.

If dategrep works on a normal file, it can do a binary search to find the first
and last line to print pretty efficiently. dategrep can also read from stdin
and compressed files, but as it can't do any seeking in those files, we have to
parse every line until we find the first falling in our date range. But at
least we don't have to wait for the pipe to be closed. As soon as we find the
first date not in the range, dategrep terminates.

# EXAMPLES

But just let me show you a few examples.

The only parameter dategrep really needs is I<format> to tell it how to
reckognize a timestamp. In this case dategrep matches all lines from epoch to
the time dategrep started. In this case it's just a glorified cat that knows
when to stop.

    dategrep -F "%b %d %H:%M:%S" syslog

Besides the format specifiers, which are very similar to the ones used
by _strptime_, dategrep knows about a few named formats like rsyslog
or apache.

    dategrep -F apache access.log

But things start to get interesting if you add the I<start> and I<end> options.

    dategrep -f 12:00 -t 12:15 --format rsyslog syslog

If you leave one out it again either defaults to epoch or now.

    dategrep  -t 12:15 -F rsyslog syslog

Pipes can also be handled, but those will be slower to filter as dategrep can't
just seek around for a pipe.  It's often more efficient to just redirect the
lines from the pipe to a file first. But nothing is stopping you to just call
dategrep directly.

    cat syslog | dategrep --end 12:15
    dategrep --end 12:15 syslog.gz

# OPTIONS

* -f DATETIME

  Print all lines from DATETIME inclusively. Defaults to Jan 1, 1970 00:00:00
  GMT.

* -t DATETIME

  Print all lines until DATESPEC exclusively. Default to the current time.

* -F FORMAT

  Defines a strftime-based FORMAT that is used to parse the input
  lines for a date. The first date found on a line is used.

  This parameter defaults to _rsyslog_.

  Additionally, dategrep supports named formats:

  * rsyslog "%b %e %H:%M:%S"
  * apache "%d/%b/%Y:%H:%M:%S"
  * iso8601 "%Y-%m-%dT%H:%M:%S"

* -h

  Shows a short help message

# LIMITATION

dategrep expects the files to be sorted. If the timestamps are not
ascending, dategrep might be exiting before the last line in its date
range is printed.

# SEE ALSO

_strftime(2)_, strptime(2)

# NOTES

This is a reimplementation of a former dategrep by me written in perl. It does
not support the same switches and does parse the same dates.

# COPYRIGHT AND LICENSE

Copyright 2014 Mario Domgoergen <mario@domgoergen.com>

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
