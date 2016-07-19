
default: build/Makefile
	cd build && make

build/Makefile:
	cd build && cmake ../

test: build/Makefile
	cd build && make testv

js: build-js/Makefile
	@cd build-js && make octocore_js

dist_js: build-js/dist/Makefile
	@cd build-js/dist && make octocore_js

test_js: build-js/Makefile
	@cd build-js && make octocore_test && cd core/ && node octocore_test.js

build-js/Makefile: EMSCRIPTEN_ROOT = $(shell python -c "import imp, os; ec=imp.load_source('ec', os.getenv('HOME')+'/.emscripten'); print(ec.EMSCRIPTEN_ROOT)")
build-js/Makefile:
	@echo EMSCRIPTEN_ROOT is $(EMSCRIPTEN_ROOT)
	cd build-js && cmake \
		-DEMSCRIPTEN_ROOT=$(EMSCRIPTEN_ROOT) \
		-DCMAKE_TOOLCHAIN_FILE=$(EMSCRIPTEN_ROOT)/cmake/Modules/Platform/Emscripten.cmake \
		-DCMAKE_BUILD_TYPE=DEBUG \
		../

build-js/dist:
	@mkdir build-js/dist

build-js/dist/Makefile: EMSCRIPTEN_ROOT = $(shell python -c "import imp, os; ec=imp.load_source('ec', os.getenv('HOME')+'/.emscripten'); print(ec.EMSCRIPTEN_ROOT)")
build-js/dist/Makefile: build-js/dist
	@echo EMSCRIPTEN_ROOT is $(EMSCRIPTEN_ROOT)
	cd build-js/dist && cmake \
		-DEMSCRIPTEN_ROOT=$(EMSCRIPTEN_ROOT) \
		-DCMAKE_TOOLCHAIN_FILE=$(EMSCRIPTEN_ROOT)/cmake/Modules/Platform/Emscripten.cmake \
		-DCMAKE_BUILD_TYPE=Release \
		../../

style: build/Makefile
	@cd build && make vera_style

build-xcode/Makefile:
	@cd build-xcode && cmake -G Xcode ../

xcode: build-xcode/Makefile
	@cd build-xcode && open "OctoCore.xcodeproj"
