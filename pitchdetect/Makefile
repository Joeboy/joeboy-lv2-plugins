BUNDLE = pitchdetect.lv2
INSTALL_DIR = /usr/local/lib/lv2


$(BUNDLE): manifest.ttl pitchdetect.ttl pitchdetect.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl pitchdetect.ttl pitchdetect.so $(BUNDLE)

pitchdetect.so: pitchdetect.c
	gcc -shared -fPIC -DPIC pitchdetect.c `pkg-config --cflags --libs lv2core aubio` -o pitchdetect.so

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) pitchdetect.so
