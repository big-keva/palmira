# 


## Module structure
```
elastic-client/
├── CMakeLists.txt
├── include/
│   └── elastic/
│       ├── http/
│       │   ├── client.h
│       │   ├── session.h
│       │   ├── request.h
│       │   ├── response.h
│       │   ├── endpoint.h
│       │   ├── method.h
│       │   ├── parameter.h
│       │   ├── request_headers.h
│       │   └── response_headers.h
│       │
│       └── api/
│           ├── client.h
│           ├── document/
│           │   ├── index_document_request.h
│           │   ├── index_document_response.h
│           │   ├── get_document_request.h
│           │   └── get_document_response.h
│           └── common/
│               ├── request_traits.h
│               └── response_base.h
│
├── src/
│   ├── http/
│   └── api/
│
└── tests/
├── document_api.t.cpp
├── endpoint.t.cpp
└── client.t.cpp
```
## Fluent HTTP API

Низкоуровневый HTTP-запрос можно выполнить через одноразовый builder:

```cpp
auto response = client
    .method(elastic::http::method::post)
    .path("books/_doc/42")
    .timeout(10000)
    .body(R"({"name":"brave","age":42})")
    .query("refresh", "wait_for")
    .accept_json()
    .content_type_json()
    .execute();
```

Также доступны сокращённые методы:

```cpp
auto response = client
    .get("books/_doc/42")
    .query("realtime", true)
    .execute();
```

`request_builder` является move-only и допускает только один вызов `execute()`.
Типизированный интерфейс `client.execute(document_request)` сохранён.


## Типизированный fluent Document API

```cpp
elastic::api::client client{"http://127.0.0.1:9200"};

const auto json = R"({"name":"brave","age":42})";

auto index_response = client
    .index("books", "42")
    .body(json)
    .query("refresh", "wait_for")
    .timeout(10000)
    .execute();

auto get_response = client
    .get_document("books", "42")
    .execute();

auto delete_response = client
    .delete_document("books", "42")
    .query("refresh", "wait_for")
    .execute();
```

`index(index)` использует `POST /{index}/_doc` и поручает OpenSearch создать ID.
`index(index, id)` использует `PUT /{index}/_doc/{id}`. Значения индекса и ID
добавляются как отдельные URI-сегменты и percent-encode-ятся автоматически.
