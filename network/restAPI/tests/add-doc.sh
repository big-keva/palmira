#!/bin/bash

# Adding a document
#curl -X POST "127.0.0.1:9300/tests/_doc" -H 'Content-Type: application/json' -d'
#{
#  "title": "Первая запись",
#  "author": "Иван Иванович",
#  "content": "Это моя первая запись в блоге."
#}
#'

curl -X PUT "127.0.0.1:57571/i1/1" -H 'Content-Type: application/json' -d'
{
  "document": {
    "title": "Первая запись",
    "author": "Иван Иванович",
    "content": "Это моя первая запись в блоге."
  },
  "metadata": {
    "lamport-clock": 997,
    "data-string": "Иван Иванович"
  },
  "version": 94,
  "condition": "$doc.version == 94"
}
'
