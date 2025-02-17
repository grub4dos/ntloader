
DOCS := header.md
DOCS += menu.md
DOCS += cmdline.md
DOCS += utils.md
DOCS += build.md
DOCS += faq.md
DOCS += license.md

all : README.md

README.md : $(DOCS)
	cat $(DOCS) > $@

clean :
	rm -f README.md
