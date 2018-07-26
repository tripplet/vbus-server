# vbus-server
[![Build Status](https://travis-ci.org/tripplet/vbus-server.svg?branch=master)](https://travis-ci.org/tripplet/vbus-server)
[![](https://img.shields.io/docker/build/ttobias/vbus-server.svg)](https://hub.docker.com/r/ttobias/vbus-server/)
[![](https://images.microbadger.com/badges/image/ttobias/vbus-server.svg)](https://microbadger.com/images/ttobias/vbus-server)
[![GitHub license](https://img.shields.io/github/license/tripplet/vbus-collector.svg)](https://github.com/tripplet/vbus-server/blob/master/LICENSE.txt)


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
$ git clone --recurse-submodules https://github.com/tripplet/vbus-server.git server
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
TIME should be the timespan of desired past data.

Example: `http://ip-of-your-raspberrypi/heating?timespan=-5 days`

For a list of supported values see: https://www.sqlite.org/lang_datefunc.html

## Additional supported API requests
* Format can be csv or json
* ?timespan=current&format=json
  ```
  {"data":
    [
      {"timestamp":"2018-07-08 11:13:01", 
        "temp1":21.9,
        "temp2":20.9,
        "temp3":23.1,
        "temp4":27.1,
        "valve1":0,
        "valve2":0
      }
    ]
  }
  ```
* ?start=2018-01-01&timespan=1 month&current&format=json
  ```
  {"data":[
      {"timestamp":"2018-01-01 00:00:01", "temp1":79.6, "temp2":73.3, "temp3":77.5, "temp4":64, "valve1":100, "valve2":100},
      {"timestamp":"2018-01-01 00:01:01", "temp1":79.5, "temp2":73.3,"temp3":77.6, "temp4":64, "valve1":100, "valve2":100},
      ...
      ...
      {"timestamp":"2018-01-31 23:59:00", "temp1":74, "temp2":63.8, "temp3":73.3, "temp4":26.5, "valve1":100, "valve2":100}
   ],
   "temp1": {"min": 28.9, "max": 94.3},
   "temp2": {"min": 24.3, "max": 85.7},
   "temp3": {"min": 59.6, "max": 91.6},
   "temp4": {"min": 17.9, "max": 74.4}
  }
  ```
    
