include $(dirs.root)/make/default.mak

include $(dirs.root)/make/build$(if $(findstring :,$(arch)$(android.minsdk)),_multivar).mak
