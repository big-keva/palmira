#!/bin/bash

curl -X POST "127.0.0.1:57571/insert" -H 'Content-Type: application/json' -d'
{
  "id": "5",
  "metadata": {
    "lamport-clock": 2
  },
  "document": {
    "name": "Виталий Булганин, наш бессменный автор",
    "age": 42,
    "penis": {
      "length": 12,
      "width": [8, 10, 12]
    },
    "metrics": [
      { "chest": 120 },
      { "stoma": 90 },
      { "arsh": 110 },
      18
    ]
  }
}
'
