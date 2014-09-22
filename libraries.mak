#common
libraries += $(libraries.common)
depends += $(foreach lib,$(libraries.common),src/$(subst _,/,$(lib)))

#3rdparty
ifdef system.zlib
libraries += $(subst zlib,z,$(libraries.3rdparty))
depends += $(foreach lib,$(filter-out zlib,$(libraries.3rdparty)),3rdparty/$(subst _,/,$(lib)))
else
libraries += $(libraries.3rdparty)
depends += $(foreach lib,$(libraries.3rdparty),3rdparty/$(subst _,/,$(lib)))
endif
