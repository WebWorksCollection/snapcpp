all install: build/Makefile
	$(MAKE) -C build $@

clean distclean:
	rm -rf build

build/Makefile: CMakeLists.txt
	mkdir -p build
	cd build && cmake .. -DCMAKE_BUILD_TYPE=Release $(addprefix -DINIT_FLAVOR=,$(INIT_FLAVOR)) -DCMAKE_C_FLAGS="$(CFLAGS)" $(addprefix -DCMAKE_C_COMPILER=,$(CC))

