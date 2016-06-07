BUNDLE = shimmer_rust.lv2
INSTALL_DIR = /usr/local/lib/lv2


$(BUNDLE): manifest.ttl shimmer-rust.ttl target/debug/libshimmer_rust.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp manifest.ttl shimmer-rust.ttl target/debug/libshimmer_rust.so $(BUNDLE)

target/debug/libshimmer_rust.so: src/shimmer_rust.rs
	cargo build

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

clean:
	rm -rf $(BUNDLE) target
