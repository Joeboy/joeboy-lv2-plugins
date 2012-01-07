BUNDLE = visual_compressor.lv2
INSTALL_DIR = /usr/local/lib/lv2


$(BUNDLE): manifest.ttl visual_compressor.ttl visual_compressor.so visual_compressor_gui.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp $^ $(BUNDLE)

visual_compressor.so: visual_compressor.cpp visual_compressor.peg
	g++ -shared -fPIC -DPIC visual_compressor.cpp `pkg-config --cflags --libs lv2-plugin` -o visual_compressor.so

visual_compressor_gui.so: visual_compressor_gui.cpp visual_compressor.peg
	g++ -shared -fPIC -DPIC visual_compressor_gui.cpp `pkg-config --cflags --libs lv2-gui` -o visual_compressor_gui.so

visual_compressor.peg: visual_compressor.ttl
	lv2peg visual_compressor.ttl visual_compressor.peg

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) visual_compressor.so visual_compressor_gui.so visual_compressor.peg
