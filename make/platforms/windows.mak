makebin_name = $(1).exe
makelib_name = $(1).lib
makedyn_name = $(1).dll
makeobj_name = $(1).obj
makeres_cmd = rc $(addprefix /d, $(DEFINES)) /r /fo$(2) $(1)

host=windows
compiler := msvs
