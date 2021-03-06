#!/usr/bin/python
#
# Minimal Printserver, forwards a printer device to a tcp port (usually 9100)
# 
# TODO: 
#   * add read for bidirectional comm ?
#   * add writeonly opts
#   * serial device support ?
#
# Copyright 2006, Canonical Ltd.
# 
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License with your
# Debian GNU system, in /usr/share/common-licenses/GPL.  If not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA

import os
import sys
import socket
import resource      # Resource usage information.

# Print usage information
if len(sys.argv) != 3:
    print 'Usage: jetpipe <printerdevice> <port>'
    sys.exit(0)

# Wait for a job on <port> to forward to <printerdevice>
def listen(s):
    conn, addr = s.accept()
    p = open(sys.argv[1], 'wb')
    while 1:
        data = conn.recv(1024)
        if not data: break
        p.write(data)
        p.flush()
    conn.close()
    p.close()

# Rewspawn eternally
if __name__ == "__main__":
    # do the UNIX double-fork magic, see Stevens' "Advanced
    # Programming in the UNIX Environment" for details (ISBN 0201563177)
    try:
        pid = os.fork()
        if pid > 0:
            # exit first parent
            sys.exit(0)
    except OSError, e:
        print >>sys.stderr, "fork #1 failed: %d (%s)" % (e.errno, e.strerror)
        sys.exit(1)

    # decouple from parent environment
    os.chdir("/")
    os.setsid()
    os.umask(0)

    # do second fork
    try:
        pid = os.fork()
        if pid > 0:
            sys.exit(0)
    except OSError, e:
        print >>sys.stderr, "fork #2 failed: %d (%s)" % (e.errno, e.strerror)
        sys.exit(1)

    maxfd = resource.getrlimit(resource.RLIMIT_NOFILE)[1]
    if (maxfd == resource.RLIM_INFINITY):
        maxfd = MAXFD
  
    # Iterate through and close all file descriptors.
    for fd in range(0, maxfd):
        try:
            os.close(fd)
        except OSError:   # ERROR, fd wasn't open to begin with (ignored)
            pass

    # Redirect the standard I/O file descriptors to the specified file.  Since
    # the daemon has no controlling terminal, most daemons redirect stdin,
    # stdout, and stderr to /dev/null.  This is done to prevent side-effects
    # from reads and writes to the standard I/O file descriptors.

    # This call to open is guaranteed to return the lowest file descriptor,
    # which will be 0 (stdin), since it was closed above.
    os.open('/dev/null', os.O_RDWR)  # standard input (0)

    # Duplicate standard input to standard output and standard error.
    os.dup2(0, 1)            # standard output (1)
    os.dup2(0, 2)            # standard error (2)

    jetpipe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    jetpipe.bind(('', int(sys.argv[2])))
    jetpipe.listen(1)

    while 1:
        listen(jetpipe)

