# Palmira server modules

## Module configuration
```
    "modules": {
        "files": [
            {
                "name": "docapi-http",
                "file": "cmake-build-debug/lib/libdocapi-http.so",
                "listen_port": 9200,
                "workers": 2,
                "max_body_size": "5M"
            },
        ]
    }
```