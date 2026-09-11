#!/bin/bash

# Getting document '1'

curl -X GET "127.0.0.1:57571/i1/1" -H 'Content-Type: application/json' -d'
{
}
'

curl -X POST "127.0.0.1:57571/get" -H 'Content-Type: application/json' -d'
{
  "id": "1"
}
'

curl -X POST "127.0.0.1:57571/get" -H 'Content-Type: application/json' -d'
{
  "id": "1",
  "space": "i1"
}
'

curl -X POST "127.0.0.1:57571/get/i1/1" -H 'Content-Type: application/json' -d'
{
}
'
curl -X GET "127.0.0.1:57571/i1/5" -H 'Content-Type: application/json' -d'
{
}
'

