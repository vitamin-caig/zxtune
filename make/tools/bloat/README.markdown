# bloat

Generate [webtreemap][]-compatible JSON summaries of binary size.

[webtreemap]: http://github.com/martine/webtreemap

## Setup

1) Check out a copy of webtreemap in a `webtreemap` subdirectory:

        git clone git://github.com/martine/webtreemap.git

2) Build your binary with the `-g` flag to get symbols.

3) Run `./bloat.py --help` and generate `nm.out` as instructed there.

4) Example command line:

        ./bloat.py --strip-prefix=/path/to/src syms > bloat.json

## Misc other feature

Dump large symbols:

    $ ./bloat.py dump | head -20
