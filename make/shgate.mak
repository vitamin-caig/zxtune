ifdef tools.perl
%_api_dynamic$(src_suffix): templates/%_api_dynamic$(src_suffix).templ functions/%.dat
		$(tools.perl) $(path_step)/make/build_shgate_dynamic.pl $^ $@

%_api.h: templates/%_api.h.templ functions/%.dat
		$(tools.perl) $(path_step)/make/build_shgate_dynamic.pl $^ $@
endif