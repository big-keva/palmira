#!/bin/bash

# DELETE /space/id
#curl -X DELETE "127.0.0.1:57571/space/1" -H 'Content-Type: application/json' -d'{}'

# GET /delete/space/id
#curl -X GET "127.0.0.1:57571/delete/space/1" -H 'Content-Type: application/json' -d'{}'

# GET /remove/space/id
#curl -X GET "127.0.0.1:57571/remove/space/1" -H 'Content-Type: application/json' -d'{}'

# POST /remove
#curl -X POST "127.0.0.1:57571/remove" -H 'Content-Type: application/json'
#curl -X POST "127.0.0.1:57571/remove?space=space" -H 'Content-Type: application/json'

#curl -X POST "127.0.0.1:57571/remove?space=space" -H 'Content-Type: application/json' -d'"1"'
#curl -X POST "127.0.0.1:57571/remove?space=space" -H 'Content-Type: application/json' -d'["1", "2", "3"]'
#curl -X POST "127.0.0.1:57571/remove?space=space" -H 'Content-Type: application/json' -d'{ "id": 1 }'
#curl -X POST "127.0.0.1:57571/remove" -H 'Content-Type: application/json' -d'["id": "1"]'
curl -X POST "127.0.0.1:57571/remove?space=space" -H 'Content-Type: application/json' -d'
[
  "1",
  {
    "id": "2"
  }
]
'
