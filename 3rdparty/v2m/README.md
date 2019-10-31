# v2m-player
Farbrausch V2M player

![TinyPlayer](https://github.com/zvezdochiot/v2m-player/blob/master/doc/V2M-TinyPlayer.jpg)

This is a quick port of the tinyplayer at https://github.com/farbrausch/fr_public/tree/master/v2 to SDL.

## Build

```
ccmake .
cmake .
make
sudo make install
```

## Usage

```
./v2mplayer v2m/0test.v2m
zcat v2m/Dafunk--breeze.v2mz | ./v2mplayer
gzip -cdf v2m/Dafunk--breeze.v2mz | ./v2mplayer -o Dafunk--breeze.newest.v2m
```

## See also

```
http://ftp.modland.com/
```

--- 
2018
