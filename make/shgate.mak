ifdef tools.perl
gates/%_api_dynamic$(suffix.cpp): templates/%_api_dynamic$(suffix.cpp).templ functions/%.dat
		$(tools.perl) $(dirs.root)/make/build_shgate_dynamic.pl $^ $@

gates/%_api.h: templates/%_api.h.templ functions/%.dat
		$(tools.perl) $(dirs.root)/make/build_shgate_dynamic.pl $^ $@
endif
