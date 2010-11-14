To install ZXTune to dingux

1) Extract this archive content to local/apps/zxtune folder
2) Copy all .qpf files local/share/fonts
3.1) In case of DMenu:
  Edit local/dmenu/themes/${ThemeName}/menu_media.cfg:

  include("/usr/local/apps/zxtune/zxtune.cfg")

  in any place inside 'Menu' section (NOT inside 'MenuItem'!!!)
3.2) In case of GMenu:
  Copy file 'zxtune' to local/gmenu2x/sections/applications/
