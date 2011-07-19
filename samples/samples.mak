install_samples:
	$(call makedir_cmd,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/ay/AYMD39.ay,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/asc/SANDRA.asc,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/pt2/Illusion.pt2,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/pt3/Speccy2.pt3,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/stc/stracker.stc,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/ts/long_day.pt3,$(DESTDIR)/Samples)
	$(call copyfile_cmd,$(path_step)/samples/pdt/TechCent.M,$(DESTDIR)/Samples)

install_samples_playlist: 
	$(call copyfile_cmd,$(path_step)/samples/samples.xspf,$(DESTDIR))
