# vbus-server
[![Build Status](https://travis-ci.org/tripplet/vbus-server.svg?branch=master)](https://travis-ci.org/tripplet/vbus-server)

![](/doc/screenshot.png?raw "Screenshot")

Data capturing is done by [vbus-collector](https://github.com/tripplet/vbus-collector)

## HowTo
The RaspberryPi or other linux machine should be running and connected to the internet, ssh sould be available.
Also vbus-collector should be running.

* Get root via `sudo -s`, `su` or other ways :smile:

Get the necessary packages (raspbian)
```shell
$ apt-get update
$ apt-get install git build-essential cmake libsqlite3-dev
```

Get the necessary packages (archlinux-arm)
```shell
$ pacman -Syu
$ pacman -S git base-devel cmake libsqlite3-dev sqlite
```

Download the source code
```shell
$ mkdir -p /opt/vbus
$ cd /opt/vbus
$ git clone https://github.com/tripplet/vbus-server.git server
$ cd server
$ git submodule update --init
```

Compile the data visualization service
```shell
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ cd ../web
$ ln -s ../build/vbus-server
```
