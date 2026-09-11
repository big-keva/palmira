#!/bin/bash

# Adding a document
#curl -X POST "127.0.0.1:9300/tests/_doc" -H 'Content-Type: application/json' -d'
#{
#  "title": "Первая запись",
#  "author": "Иван Иванович",
#  "content": "Это моя первая запись в блоге."
#}
#'

curl -X PATCH "127.0.0.1:57571/i1/1" -H 'Content-Type: application/json' -d'
{
  "metadata": {
    "lamport-clock": 997,
    "data-string": "Иван Иванович"
  },
  "condition": "$doc.version == 94"
}
'

curl -X PATCH "127.0.0.1:57571/i1" -H 'Content-Type: application/json' -d'
{
  "id": "1",
  "metadata": {
    "lamport-clock": 997,
    "data-string": "Иван Иванович"
  },
  "condition": "$doc.version == 94"
}
'

curl -X PATCH "127.0.0.1:57571" -H 'Content-Type: application/json' -d'
{
  "id": "1",
  "metadata": {
    "lamport-clock": 997,
    "data-string": "Иван Иванович"
  },
  "condition": "$doc.version == 94"
}
'
