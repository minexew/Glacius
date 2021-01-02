# Ubuntu 20.04 provides GCC 10.2
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y cmake libmysqlclient-dev gcc g++
RUN gcc -v

COPY . /work

WORKDIR /work
RUN cd /work && cmake . && make VERBOSE=1 Glacius GlaciusRCC


FROM ubuntu:20.04
RUN apt-get update && apt-get install -y libmysqlclient21

COPY --from=0 /work/bin /srv/glacius
WORKDIR /srv/glacius
ENTRYPOINT ./Glacius
