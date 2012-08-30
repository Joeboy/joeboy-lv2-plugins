BUNDLE = shimmer.lv2
INSTALL_DIR = /usr/local/lib/lv2


$(BUNDLE): manifest.ttl shimmer.ttl shimmer.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl shimmer.ttl shimmer.so $(BUNDLE)

shimmer.so: shimmer.c
	gcc -shared -std=c99 -fPIC -DPIC shimmer.c -o shimmer.so

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) shimmer.so
