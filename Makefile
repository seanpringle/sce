
dev:
	rm -rf build
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build -G "Unix Makefiles"
	$(MAKE) -C build

linux:
	rm -rf build
	cmake -DCMAKE_BUILD_TYPE=Release -S . -B build -G "Unix Makefiles"
	$(MAKE) -C build

clean:
	rm -rf build
	rm -f /tmp/sce.prof

prof: linux
	LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so.0 CPUPROFILE=/tmp/sce.prof build/sce
	google-pprof --web build/sce /tmp/sce.prof
