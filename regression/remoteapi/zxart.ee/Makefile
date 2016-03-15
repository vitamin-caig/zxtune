all: authors.err authors.tracks topTunes.err parties.err toptunes.err

clean:
	rm -f *.xml *.err *.ids

authors.tracks: authors.ids
	for id in $$(cat $^); do $(MAKE) author.$${id}.err ; done

parties.tracks: parties.ids
	for id in $$(cat $^); do $(MAKE) party.$${id}.err ; done

author.%.xml:
	wget http://zxart.ee/zxtune/action:tunes/authorId:$* -O $@

party.%.xml:
	wget http://zxart.ee/zxtune/action:tunes/partyId:$* -O $@

toptunes.xml:
	wget http://zxart.ee/zxtune/action:topTunes -O $@

%.xml:
	wget http://zxart.ee/zxtune/action:$* -O $@

%.err: %.xml
	xmllint --schema zxart.xsd $^ > $@ 2>&1 && rm $@

%.ids: %.xml
	sed -nr 's/.*id>(.*?)<\/id>.*/\1/pg' $^ | sort > $@
