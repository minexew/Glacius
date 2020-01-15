# Ubuntu 18.04 provides GCC 7.4
FROM ubuntu:18.04

RUN apt-get update && apt-get install -y cmake libmysqlclient-dev gcc g++
RUN gcc -v

COPY . /work

WORKDIR /work
RUN cd /work && cmake . && make VERBOSE=1 Glacius GlaciusRCC


FROM ubuntu:18.04
RUN apt-get update && apt-get install -y libmysqlclient20

COPY --from=0 /work/bin /srv/glacius
WORKDIR /srv/glacius
ENTRYPOINT ./Glacius
