#!/bin/bash

readonly INSTANCE_NAME="test_elastic"
readonly HTTP_PORT="9200"
readonly ELASTIC_VERSION="9.4.3"
readonly ELASTIC_NET_NAME="elastic"

readonly ELASTIC_IMAGE="docker.elastic.co/elasticsearch/elasticsearch:${ELASTIC_VERSION}"

cleanup_on_error() {
  local exit_code=$?

  echo "Ошибка запуска Elasticsearch, код: ${exit_code}" >&2
  docker logs "${INSTANCE_NAME}" 2>/dev/null || true

  exit "${exit_code}"
}

trap cleanup_on_error ERR

#
# Создаём сеть только в том случае, если она ещё не существует.
#
if ! docker network inspect "${ELASTIC_NET_NAME}" >/dev/null 2>&1; then
  docker network create "${ELASTIC_NET_NAME}"
fi

#
# Удаляем ранее созданный тестовый контейнер.
# Данные в этом варианте не сохраняются.
#
if docker container inspect "${INSTANCE_NAME}" >/dev/null 2>&1; then
  docker rm -f "${INSTANCE_NAME}"
fi

#
# Загружаем конкретную версию образа.
#
docker pull "${ELASTIC_IMAGE}"

#
# Запускаем од-node Elasticsearch.
#
# xpack.security.enabled=false используется только для локальных тестов.
# В production отключать security нельзя.
#
docker run \
  --detach \
  --name "${INSTANCE_NAME}" \
  --network "${ELASTIC_NET_NAME}" \
  --publish "127.0.0.1:${HTTP_PORT}:9200" \
  --env "discovery.type=single-node" \
  --env "xpack.security.enabled=false" \
  --env "ES_JAVA_OPTS=-Xms1g -Xmx1g" \
  "${ELASTIC_IMAGE}"

for attempt in $(seq 1 60); do
  echo "[$attempt] Ожидание готовности Elasticsearch..."
  if curl --silent --fail "http://127.0.0.1:${HTTP_PORT}/_cluster/health" >/dev/null; then
    echo "ElasticSearch сервер готов к работе:"
    curl --silent "http://127.0.0.1:${HTTP_PORT}/" | python3 -m json.tool
    exit 0
  fi

  if ! docker container inspect --format '{{.State.Running}}' "${INSTANCE_NAME}" 2>/dev/null | grep -q '^true$'; then
    echo "Контейнер Elasticsearch остановился." >&2
    docker logs "${INSTANCE_NAME}" >&2
    exit 1
  fi

  sleep 2
done

echo "Elasticsearch не запустился за отведённое время." >&2
docker logs "${INSTANCE_NAME}" >&2

exit 1