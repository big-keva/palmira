#!/bin/bash

# Adding a document
curl -X POST "127.0.0.1:9300/tests/_doc/1" -H 'Content-Type: application/json' -d '
{
  "title": "Первая запись",
  "author": "Иван Иванович",
  "content": "Это моя первая запись в блоге.",
  "annotation": {
    "name": "Vitaly",
    "identity": {
      "type": "pasport"
    }
  }
}
'
DOCAPI_DOC_ID=1

echo "Getting a document by id: ${DOCAPI_DOC_ID}"
curl -X POST "127.0.0.1:9300/tests/_doc/${DOCAPI_DOC_ID}"

echo "Getting a document by id and title: ${DOCAPI_DOC_ID}"
curl -X POST "127.0.0.1:9300/tests/_doc/${DOCAPI_DOC_ID}?title=Первая запись"

echo "Getting a document by id and _source: ${DOCAPI_DOC_ID}"
curl -X POST "127.0.0.1:9300/tests/_doc/${DOCAPI_DOC_ID}?_source=Первая запись"
