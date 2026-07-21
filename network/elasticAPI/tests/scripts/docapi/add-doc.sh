#!/bin/bash

# Adding a document
curl -X POST "127.0.0.1:9300/tests/_doc" -H 'Content-Type: application/json' -d'
{
  "title": "Первая запись",
  "author": "Иван Иванович",
  "content": "Это моя первая запись в блоге.",
  "reviewers": [
    {
      "name": "Vitaly",
      "position": "staff"
    },
    {
      "name": "Kaplan",
      "position": "staff"
    }
  ]
}
'

curl -X PUT "127.0.0.1:9300/tests/_doc/1" -H 'Content-Type: application/json' -d'
{
  "title": "Первая запись",
  "author": "Иван Иванович",
  "content": "Это моя первая запись в блоге."
}
'
