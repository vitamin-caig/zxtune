#!/bin/bash
# Fetch a URL via Firecrawl, converting to the selected format.
# Usage: fetch_url.sh [--mode markdown|raw] <url>
#   markdown (default) - Firecrawl markdown conversion
#   raw                - raw content (rawHtml), preserves XML tags/whitespace
#                         for maven-metadata.xml and other machine-readable docs
mode=markdown
url=

while [ "$#" -gt 0 ]; do
  case "$1" in
    --mode)
      mode="$2"
      shift 2
      ;;
    *)
      url="$1"
      shift
      ;;
  esac
done

test -n "$url" || { echo "Usage: fetch_url.sh [--mode markdown|raw] <url>" >&2; exit 1; }

case "$mode" in
  markdown)
    format=markdown
    ;;
  raw)
    format=rawHtml
    ;;
  *)
    echo "Unknown mode: $mode" >&2
    exit 2
    ;;
esac

curl -s -X POST "https://api.firecrawl.dev/v2/scrape" \
  -H 'Content-Type: application/json' \
  -d "{\"url\": \"${url}\", \"formats\": [\"${format}\"]}"