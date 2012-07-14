BUNDLE = fluidsynth.lv2
INSTALL_DIR = /usr/local/lib/lv2


$(BUNDLE): manifest.ttl fluidsynth.ttl fluidsynth.so fluidsynth_ui.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl fluidsynth.ttl fluidsynth.so fluidsynth_ui.so $(BUNDLE)

fluidsynth.so: fluidsynth.c
	gcc -std=c99 -shared -fPIC -DPIC fluidsynth.c `pkg-config --cflags --libs lv2-plugin fluidsynth` -o fluidsynth.so -lm

fluidsynth_ui.so: fluidsynth_ui.c uris.h
	gcc -g -std=c99 -shared -fPIC -DPIC fluidsynth_ui.c `pkg-config --cflags --libs gtk+-2.0` -o fluidsynth_ui.so


install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) *.so
