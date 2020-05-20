#!/bin/sh
echo Building mojo2600/esp-soil-moisture-sensor:build

docker build --build-arg NODE_ENV="production" -t mojo2600/esp-soil-moisture-sensor:build .

docker container create --name extract mojo2600/esp-soil-moisture-sensor:build  
docker container cp extract:/app/dist/. ../data/
docker container rm -f extract
