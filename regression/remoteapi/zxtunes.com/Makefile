all: authors.err authors.tracks

clean:
	rm *.err *.xml *.ids *.tmp

download = wget $(1) -O $(2).tmp && xmllint --format $(2).tmp > $(2) 2>&1 && rm -f $(2).tmp || mv $(2).tmp $(2)

authors.tracks: authors.ids
	for id in $$(cat $^); do $(MAKE) author.$${id}.err ; done

authors.xml:
	$(call download,"http://zxtunes.com/xml.php?scope=authors&fields=nickname%2Cname%2Ctracks",$@)

author.%.xml:
	$(call download,"http://zxtunes.com/xml.php?scope=tracks&fields=filename%2Ctitle%2Cduration%2Cdate&author_id=$*",$@)

%.err: %.xml
	xmllint --schema zxtunes.xsd $^ > $@ 2>&1 && rm $@

%.ids: %.xml
	sed -nr 's/.*id=.([0-9]+).*/\1/pg' $^ | sort > $@
