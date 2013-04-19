#common
libraries += $(libraries.common)
depends += $(foreach lib,$(libraries.common),src/$(subst _,/,$(lib)))

#3rdparty
libraries += $(libraries.3rdparty)
depends += $(foreach lib,$(libraries.3rdparty),3rdparty/$(subst _,/,$(lib)))
