samples_path := $(path_step)/samples/chiptunes

samples_ay38910 := \
	as0/Samba.as0 \
	asc/SANDRA.asc \
	ay/AYMD39.ay \
	ftc/Nostalgy.ftc \
	gtr/L.Boy.gtr \
	psc/FL_SH_EI.psc \
	psg/Illusion.psg \
	psm/Calamba.psm \
	pt1/GoldenGift.pt1 \
	pt2/PITON.pt2 \
	pt3/Speccy2.pt3 \
	sqt/taiobyte.sqt \
	st1/EPILOG.st1 \
	st3/Kvs_Joke.st3 \
	stc/stracker.stc \
	stp/iris_setup.stp \
	ts/INEEDREST.ts \
	txt/C_IMPROV.txt \
	vtx/Enlight3.vtx \
	ym/Kurztech.ym

samples_ay38910_dir := AY-3-8910

samples_dac_zx := \
	chi/ducktale.chi \
	dmm/popcorn.dmm \
	dst/Untitled.dst \
	pdt/TechCent.M \
	sqd/Maskarad.sqd \
	str/COMETDAY.str

samples_dac_zx_dir := DAC/ZX

samples_saa1099 := \
	cop/axel.cop

samples_saa1099_dir := SAA1099

samples_ym2203 := \
	tf0/ducktales1_moon.tf0 \
	tfc/unbeliev.tfc \
	tfd/mass_production.tfd \
	tfe/disco.tfe

samples_ym2203_dir := YM2203

install_samples: install_samples_ay38910 install_samples_dac_zx install_samples_saa1099 install_samples_ym2203

install_samples_ay38910:
	$(call makedir_cmd,$(DESTDIR)/Samples/$(samples_ay38910_dir))
	$(foreach smp,$(samples_ay38910),$(call copyfile_cmd,$(samples_path)/$(samples_ay38910_dir)/$(smp),$(DESTDIR)/Samples/$(samples_ay38910_dir)) && ) \
	echo Installed AY-3-8910 samples

install_samples_dac_zx:
	$(call makedir_cmd,$(DESTDIR)/Samples/$(samples_dac_zx_dir))
	$(foreach smp,$(samples_dac_zx),$(call copyfile_cmd,$(samples_path)/$(samples_dac_zx_dir)/$(smp),$(DESTDIR)/Samples/$(samples_dac_zx_dir)) && ) \
	echo Installed DAC/ZX samples

install_samples_saa1099:
	$(call makedir_cmd,$(DESTDIR)/Samples/$(samples_saa1099_dir))
	$(foreach smp,$(samples_saa1099),$(call copyfile_cmd,$(samples_path)/$(samples_saa1099_dir)/$(smp),$(DESTDIR)/Samples/$(samples_saa1099_dir)) && ) \
	echo Installed SAA1099 samples

install_samples_ym2203:
	$(call makedir_cmd,$(DESTDIR)/Samples/$(samples_ym2203_dir))
	$(foreach smp,$(samples_ym2203),$(call copyfile_cmd,$(samples_path)/$(samples_ym2203_dir)/$(smp),$(DESTDIR)/Samples/$(samples_ym2203_dir)) && ) \
	echo Installed YM2203 samples

install_samples_playlist: 
	$(call copyfile_cmd,$(path_step)/samples/samples.xspf,$(DESTDIR))
