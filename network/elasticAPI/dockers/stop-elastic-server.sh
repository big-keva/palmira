#!/bin/bash

docker stop test_elastic && docker rm -f test_elastic
docker network rm elastic

exit 0
