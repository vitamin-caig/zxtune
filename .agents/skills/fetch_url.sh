#!/bin/bash
test -n "${1}" && curl -s -X POST "https://api.firecrawl.dev/v2/scrape" \
  -H 'Content-Type: application/json' \
  -d "{\"url\": \"${1}\", \"formats\": [\"markdown\"] }"
