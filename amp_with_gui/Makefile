BUNDLE = amp_with_gui.lv2
INSTALL_DIR = /usr/local/lib/lv2

$(BUNDLE): manifest.ttl amp.ttl amp.so amp_gui.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp $^ $(BUNDLE)

amp.so: amp.c
	gcc -shared -fPIC -DPIC amp.c -o amp.so

amp_gui.so: amp_gui.c
	gcc -shared -fPIC -DPIC amp_gui.c `pkg-config --cflags --libs gtk+-2.0` -o amp_gui.so

clean:
	rm -rf $(BUNDLE) *.so
