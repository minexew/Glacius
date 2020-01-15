FROM ubuntu:18.04

RUN apt-get update && apt-get install -y cmake libmysqlclient-dev gcc g++
RUN gcc -v

COPY . /work

WORKDIR /work
RUN cd /work && cmake . && make VERBOSE=1 Glacius GlaciusRCC

WORKDIR /work/bin
ENTRYPOINT ./Glacius

