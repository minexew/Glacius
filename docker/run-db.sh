#!/bin/sh
set -ex
mkdir -p mariadb-data
env CURRENT_UID=$(id -u):$(id -g) docker-compose up --build --abort-on-container-exit mariadb phpmyadmin
