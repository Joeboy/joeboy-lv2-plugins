BUNDLE = fluidsynth.lv2
INSTALL_DIR = /usr/local/lib/lv2


$(BUNDLE): manifest.ttl fluidsynth.ttl fluidsynth.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl fluidsynth.ttl fluidsynth.so $(BUNDLE)

fluidsynth.so: fluidsynth.c
	gcc -std=c99 -shared -fPIC -DPIC fluidsynth.c `pkg-config --cflags --libs lv2-plugin fluidsynth` -o fluidsynth.so -lm

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) fluidsynth.so
