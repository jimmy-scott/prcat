prcat - Tunnel (or pipe) data over TCP through an HTTP proxy
------------------------------------------------------------

The primary purpose of the prcat utility is to tunnel the git protocol
through an http proxy, but it should be able to tunnel most applications
that want to read/write from one or two file descriptors.

Works on Linux and OSX, so should work on BSD too. Solaris needs work.

How it works
============

By default it reads from stdin and writes to stdout, but this can be
changed by using the -I and -O options (or --input-fd and --output-fd).

The default config file is ~/.prcat, which can be overruled with the -f
flag (or --filename). The proxy hostname and proxy port must be present
in the config file or on the command line.

Proxy username and password are optional. If the username is provided,
but no password, the password will be asked on your terminal. This is
a main feature I wanted (otherwise I would have used netcat or so).

Command line options
====================

Following options are available (check -h or --help):

    -f <filename>
    --filename <filename>       Alternative config file
    
    -u <username>
    --username <username>       Username for proxy authentication
    
    -p <password>
    --password <password>       Password for proxy authentication
    
    -H <proxy-host>
    --proxy-host <proxy-host>   Proxy server hostname or address
    
    -P <proxy-port>
    --proxy-port <proxy-port>   Proxy server port
    
    -I <fd>
    --input-fd <fd>             Input file descriptor
    
    -O <fd>
    --output-fd <fd>            Output file descriptor
    
    -h
    --help                      Show help (shows short options only)
    
    -v
    --version                   Show version

Configuration file options
==========================

Most command line options can be put in the config file. An example
config file containing all possible options looks like this:

    proxy-host = "myproxy"
    proxy-port = 8080
    username = "NTDOMAIN\myuser"
    password = "YouDon'tPutThisStuffInAFile!!"
    # oh, and neither on the command line :-)
    input-fd = 0
    output-fd = 1

Compile and install
===================

This should work if you cloned the git repository:

    $ make

If you didn't you need to provide a build version, if you are not sure
what the build version should be, you are safe with this:

    $ make BUILD_VERSION=UNKNOWN

To install it, copy the binary to a bindir:

    $ sudo cp src/prcat /usr/local/bin/prcat

Git-proxy configuration (git protocol)
======================================

To use it as a git proxy, make a config file with at least a username,
the proxy-host and the proxy-port. An alternative is using a wrapper
script that runs prcat with the command line switches you want.

Once you have the config file in place, configure it like this:

    $ git config --global core.gitproxy /path/to/prcat

This should be it. You can now verify it by fetching my repo:

    $ git clone git://github.com/jimmy-scott/prcat

Questions
=========

Questions? Requests? Bugs? You can find my email in any source file.

