## How to run this crap

```
podman build -t glacius .
cd docker
mkdir mariadb-data
env CURRENT_UID=0:0 podman-compose -p glacius up --build --abort-on-container-exit
```

## Installation

On Fedora, install package `mariadb-connector-c-devel`.

## Maintenance

### Provided docker-compose

phpMyAdmin is available on port 10081

To import DB schema:

`mysql -h 127.0.0.1 -u glacius -ppassword -D realm <DB-r01.sql`

### RCC

`podman exec -it glacius_gameserver_1 ./GlaciusRCC localhost`

hmm how to register?
