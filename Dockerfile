FROM ubuntu

# Install build packages
RUN apt-get update
RUN apt-get -y install git

# Add git repository source code
RUN git clone https://github.com/tripplet/vbus-server.git --recursive --branch master --depth 1 /src

RUN apt-get -y install build-essential cmake autoconf libtool libsqlite3-dev

RUN cd /src && cmake -DCMAKE_BUILD_TYPE=Release . && make -j && strip /src/vbus-server

#### Stage 2
FROM ubuntu
LABEL maintainer="Tobias Tangemann"
COPY --from=0 /src/vbus-server /bin/vbus-server

