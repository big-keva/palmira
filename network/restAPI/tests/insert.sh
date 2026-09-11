#!/bin/bash

curl -X PUT "127.0.0.1:57571/i1/1" -H 'Content-Type: application/json' -d'
{
  "document": {
    "title": "Первая запись",
    "author": {
      "name": "Андрей",
      "surname": "Коваленко"
    },
    "content": [
      "Это рецепт шашлыка.",
      {
        "source": [
          "мясо",
          "угли",
          "соль"
        ],
        "time": "1h"
      },
      "Рекомендуется с чачей"
    ],
    "locus": {
      "city": "Moscow",
      "zip": 117393
    }
  },
  "metadata": {
    "lamport-clock": 997,
    "author": "Андрей Коваленко"
  },
  "condition": "$doc.version == 94"
}
'

curl -X POST "127.0.0.1:57571/insert/i1/2" -H 'Content-Type: application/json' -d'
{
  "document": {
    "title": "Вторая запись",
    "author": {
      "name": "Андрей",
      "surname": "Коваленко"
    },
    "content": "Просто анекдот про гусей.",
    "locus": {
      "city": "Moscow",
      "zip": 117393
    }
  },
  "metadata": {
    "lamport-clock": 999,
    "author": "Андрей Коваленко"
  }
}
'

curl -X POST "127.0.0.1:57571/insert/i1" -H 'Content-Type: application/json' -d'
{
  "id": "3",
  "document": {
    "title": "Третья запись",
    "author": {
      "name": "Виталий",
      "surname": "Булганин"
    }
  },
  "metadata": {
    "lamport-clock": 1003,
    "author": "Виталий Булганин"
  }
}
'

curl -X POST "127.0.0.1:57571/insert" -H 'Content-Type: application/json' -d'
{
  "id": "4",
  "document": {
    "title": "Четвёртая запись",
    "author": {
      "name": "Виталий",
      "surname": "Булганин, Анатольевич по батюшке"
    }
  },
  "metadata": {
    "lamport-clock": 1004,
    "author": "Vitaly Bulganin"
  }
}
'

curl -X POST "127.0.0.1:57571/insert" -H 'Content-Type: application/json' -d'
{
  "id": "6",
  "document": { "name": "brave", "age": 42 }
}
'
