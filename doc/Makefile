all:
	./build.sh
	doxygen
	
install:
	mkdir -p $(DESTDIR)/usr/share/doc/
	cp -r reference $(DESTDIR)/usr/share/doc/libtoad-doc
	
clean:
	rm -rf doxylinks
	rm -rf reference/html
