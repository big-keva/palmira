#!/bin/bash

# Getting document '1'
#
#curl -X GET "127.0.0.1:57571/search?query=Булганин" -H 'Content-Type: application/json' -d'
#{
#}
#'

curl -X POST "127.0.0.1:57571/search" -H 'Content-Type: application/json' -d'
{
  "first": 1,
  "count": 10,
  "query": "Булганин"
}
'
