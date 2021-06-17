
dev:
	rm -rf build
	cmake -DCMAKE_BUILD_TYPE=Debug S . -B build -G "Unix Makefiles"
	$(MAKE) -C build

linux:
	rm -rf build
	cmake -DCMAKE_BUILD_TYPE=Release S . -B build -G "Unix Makefiles"
	$(MAKE) -C build

clean:
	rm -rf build
	rm -f /tmp/ed1.prof

prof: dev
	LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so.0 CPUPROFILE=/tmp/ed1.prof build/ed1
	google-pprof --web build/ed1 /tmp/ed1.prof
