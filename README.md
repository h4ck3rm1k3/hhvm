# HipHop for PHP

HipHop is a source code transformer which transforms PHP source code into highly optimized C++ and then compiles it using g++. Currently supported platforms are Linux and FreeBSD. There is no OS X support.

* [Developer Mailing List](http://groups.google.com/group/hiphop-php-dev)
* [Wiki](http://wiki.github.com/facebook/hiphop-php)
* [Issue Tracker](http://github.com/facebook/hiphop-php/issues)

## Required Packages

The latest information is available on the [wiki](http://wiki.github.com/facebook/hiphop-php/building-and-installing)

* cmake *2.6 is the minimum version*
* g++/gcc *4.3 is the minimum version*
* Boost *1.37 is the minimum version*
* flex
* bison
* re2c
* libmysql
* libxml2
* libmcrypt
* libicu *4.2 is the minimum version*
* openssl
* binutils
* libcap
* gd
* zlib
* tbb *Intel's Thread Building Blocks*
* [Oniguruma](http://www.geocities.jp/kosako3/oniguruma/)
* libpcre
* libexpat
* libmemcached

The following packages have had slight modifications added to them. Patches are provided and should be made against the current source copies.

* [libcurl](http://curl.haxx.se/download.html)
* src/third_party/libcurl.fb-changes.diff
* [libevent 1.4](http://www.monkey.org/~provos/libevent/)
* src/third_party/libevent-1.4.13.fb-changes.diff	OR src/third_party/libevent-1.4.14.fb-changes.diff

## Installation

You may need to point CMake to the location of your custom libcurl and libevent, or to any other libraries which needed to be installed. The *CMAKE_PREFIX_PATH* variable is used to hint to the location.

    export CMAKE_PREFIX_PATH=/home/user

To build HipHop, use the following:

Linux:

    cd /home/user/dev
    git clone git://github.com/facebook/hiphop-php.git
    cd hiphop-php
    git submodule init
    git submodule update
    export HPHP_HOME=`pwd`
    export HPHP_LIB=`pwd`/bin
    cmake .

If you are using FreeBSD instead use export - setenv

Once this is done you can generate the build file. This will return you to the shell. Finally, to build, run `make`. If any errors occur, it may be required to remove the CMakeCache.txt directory in the checkout.

    make

## Contributing to HipHop
HipHop is licensed under the PHP and Zend licenses expect as otherwise noted.

Before changes can be accepted a [Contributors Licensing Agreement](http://developers.facebook.com/opensource/cla) must be signed and returned.

## Running HipHop

Please see [the wiki page](http://wiki.github.com/facebook/hiphop-php/running-hiphop)


## Debian Build 
sudo apt-get install debootstrap

mkdir ~/debian_squeeze/

sudo debootstrap squeeze squeeze

sudo mount --bind /proc/ proc/

sudo mount --bind /dev dev

sudo mount --bind /dev/pts dev/pts

sudo chroot .

------ inside of debian


apt-get update

apt-get install gcc

apt-get install build-essential git


git clone git://github.com/h4ck3rm1k3/hiphop-php.git

mv hiphop-php hiphop-php-0.1

apt-get install debhelper cmake libtbb-dev libmcrypt-dev re2c binutils-dev libonig-dev libmysqlclient15-dev libgd2-xpm-dev libmemcached-dev libboost-all-dev libpcre3-dev libevent-dev libboost-all-dev libxml2-dev libbz2-dev libncurses-dev libreadline-dev libc-client2007e-dev libcap-dev  autoconf automake autotools-dev bison curl flex libapache2-mod-php5 libapr1 libaprutil1  libaprutil1-dbd-sqlite3 libaprutil1-ldap libcurl4-openssl-dev libltdl-dev libltdl7 libmhash2 libqdbm14 libssh2-1-dev libtool m4 mcrypt php5-cli php5-common   php5-dev php5-suhosin shtool ssl-cert emacs23-nox

#build the deb
dpkg-buildpackage -uc -us -nc
