# vbus-server
[![Build Status](https://travis-ci.org/tripplet/vbus-server.svg?branch=master)](https://travis-ci.org/tripplet/vbus-server)
[![](https://img.shields.io/docker/build/ttobias/vbus-server.svg)](https://hub.docker.com/r/ttobias/vbus-server/)
[![](https://images.microbadger.com/badges/image/ttobias/vbus-server.svg)](https://microbadger.com/images/ttobias/vbus-server)
[![GitHub license](https://img.shields.io/github/license/tripplet/vbus-collector.svg)](https://github.com/tripplet/vbus-server/blob/master/LICENSE.txt)


![](/doc/screenshot.png?raw "Screenshot")

Data capturing is done by [vbus-collector](https://github.com/tripplet/vbus-collector)

**The easiest way to use this project if Homeassistant is already used is to install the Addon [Hassio VBUS](https://github.com/tripplet/hassio-vbus)**

## Docker image
https://hub.docker.com/r/ttobias/vbus-server/

## How to setup
The RaspberryPi or other linux machine should be running and connected to the internet, ssh sould be available.
Also vbus-collector should be running.

* Get root via `sudo -s`, `su` or other ways :smile:

Get the necessary packages (raspbian)
```shell
apt-get update
apt-get install git build-essential cmake libsqlite3-dev
```

Get the necessary packages (archlinux-arm)
```shell
pacman -Syu
pacman -S git base-devel cmake libsqlite3-dev sqlite
```

Download the source code
```shell
mkdir -p /srv/vbus
cd /srv/vbus
git clone --recurse-submodules https://github.com/tripplet/vbus-server.git server
```

Compile the data visualization service
```shell
cd /srv/vbus/server/
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
nice cmake --build build --parallel
ln -s /srv/vbus/server/build/vbus-server /srv/vbus/server/web/
```

## Configure nginx or any other webserver with cgi support

#### :warning: Warning
> This is a very basic setting without authentication and https (only for internal home network).
> It should not be made accessible from the internet


Install nginx (raspbian)
```shell
apt-get install nginx fcgiwrap
```

Install nginx (archlinux-arm)
```shell
pacman -S nginx-mainline fcgiwrap
```

Add vbus-server directory to webspace root

```shell
mkdir -p /srv/http/htdocs
mkdir -p /srv/http/data
chown -R http:http /srv/http
ln -s /srv/vbus/collector/data.db /srv/http/data/vbus.sqlite
ln -s /srv/vbus/server/web /srv/http/htdocs/heating
```

On raspbian the nginx user is `www-data`
```
chown -R www-data:www-data /srv/http
```


<details>
  <summary>Example nginx configuration</summary>

```cfg
# nginx config

user http; # user www-data on raspbian
worker_processes auto;
pid /run/nginx.pid;

error_log /var/log/nginx/error.log;

events {
    worker_connections 1024;
}

http {
    include mime.types;
    default_type application/octet-stream;

    charset utf-8;
    index index.html index.htm;

    proxy_buffering off;
    client_max_body_size 0;
    fastcgi_buffers 64 4K;
    types_hash_max_size 4096;
    sendfile on;
    tcp_nopush  on;
    tcp_nodelay on;
    aio threads;
    server_tokens off;

    gzip on;
    gzip_types application/javascript text/css;

    server {
        listen 80 deferred default_server;
        listen [::]:80 deferred default_server;
        server_name _;

        root /srv/http/htdocs;

        location / {
            autoindex on;
            autoindex_exact_size off;

            try_files $uri $uri/ =404;
        }

        location ~ ^(/heating/vbus-server\.cgi)(.*)$ {
            try_files $uri =404;
            include fastcgi.conf;

            fastcgi_split_path_info  ^(.+\.cgi)(.*)$;
            fastcgi_pass unix:/run/fcgiwrap.sock; # must be unix:/run/fcgiwrap.socket on raspbian
        }
    }
}
```
</details>

Enable and start the nginx server and fastcgi wrapper
```shell
systemctl enable nginx
systemctl start nginx
systemctl enable fcgiwrap
```

Very that vbus-server cgi is working by executing:
```shell
curl "http://localhost/heating/vbus-server.cgi?timespan=current&format=json"
```

The response should contain the current timestamp and data.

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

## Used libraries
* [SQLiteC++](http://srombauts.github.io/SQLiteCpp/)
* [uriparser](http://uriparser.sourceforge.net/)
* [brotli](https://github.com/google/brotli/)
