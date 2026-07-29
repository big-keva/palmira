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