# vbus-server
[![Build Status](https://travis-ci.org/tripplet/vbus-server.svg?branch=master)](https://travis-ci.org/tripplet/vbus-server)

![](/doc/screenshot.png?raw "Screenshot")

Data capturing is done by [vbus-collector](https://github.com/tripplet/vbus-collector)

## Used libraries
* [SQLiteC++](http://srombauts.github.io/SQLiteCpp/)
* [uriparser](http://uriparser.sourceforge.net/)

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
$ git clone --recursive https://github.com/tripplet/vbus-server.git server
```

Compile the data visualization service
```shell
$ mkdir -p /opt/vbus/server/build
$ cd /opt/vbus/server/build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ ln -s /opt/vbus/server/build/vbus-server /opt/vbus/server/web/
```

## Configure lighttpd or any other webserver with cgi support

#### :warning: Warning
> This is a very basic setting without authentication and https (only for internal home network).
> It should not be made accessible from the internet


Install lighttpd (raspbian)
```shell
$ apt-get install lighttpd
```

Install lighttpd (archlinux-arm)
```shell
$ pacman -S lighttpd
```

Add vbus-server directory to webspace root

```shell
$ mkdir -p /srv/http/htdocs
$ mkdir -p /srv/http/data
$ chown -R http:http /srv/http
$ ln -s /opt/vbus/collector/data.db /srv/http/data/vbus.sqlite
$ ln -s /opt/vbus/server/web /srv/http/htdocs/heating
```

On raspbian the lighttpd user is `www-data`
```
$ chown -R www-data:www-data /srv/http
```

Example lighttpd config
```cfg
# lighttpd config

var.config_dir  = "/etc/lighttpd/"
var.server_root = "/srv/http/"

server.port = 80
server.username  = "http" # Use www-data on raspbian
server.groupname = "http" # Use www-data on raspbian
server.document-root = server_root + "htdocs"
server.errorlog      = "/var/log/lighttpd/error.log"

dir-listing.activate = "enable"
dir-listing.encoding = "utf-8"

index-file.names  = ( "index.html", "index.htm" )
mimetype.assign   = ( ".html" => "text/html",
                      ".htm"  => "text/html",
                      ".txt"  => "text/plain",
                      ".css"  => "text/css",
                      ".js"   => "application/x-javascript",
                      ".jpg"  => "image/jpeg",
                      ".svg"  => "image/svg+xml",
                      ".jpeg" => "image/jpeg",
                      ".gif"  => "image/gif",
                      ".png"  => "image/png",
                      ""      => "application/octet-stream" )
# Modules
server.modules += ( "mod_cgi" )

# CGI
cgi.assign        = ( "/vbus-server" => "" )
```

Enable and start the lighttpd server (only works with installed systemd)
```shell
$ systemctl enable lighttpd
$ systemctl start lighttpd
```

Access the website via `http://ip-of-your-raspberrypi/heating`
By default the last 12 hours are rendered, if more history is desired change the url according to this format:

```url
http://ip-of-your-raspberrypi/heating?timespan=TIME
```
TIME should be the timespan of desired past data (remember whitespace should be encoded as `%20`.

Example: `http://ip-of-your-raspberrypi/heating?timespan=-5%20days`

For a list of supported values see: https://www.sqlite.org/lang_datefunc.html
