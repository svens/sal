# Variables that select action if defined {{{1
#

# if has value, edit module
edit :=


# Variable based action selection {{{1
#


ifneq ($(edit),)
all: .edit
else
all: .build
endif


# Session launching targets (tmux) {{{1
#


session-edit:
	tmux has-session -t "sal/edit" >/dev/null 2>&1 \
	  && ( tmux detach-client -s "sal/edit" || true ) \
	  || ( \
	    tmux new-session -s "sal/edit" -n main -d \
	    && tmux select-window -t "sal/edit:main" \
	  )
	tmux attach-session -t "sal/edit"


session-work:
	tmux has-session -t "sal/work" >/dev/null 2>&1 \
	  && ( tmux detach-client -s "sal/work" || true ) \
	  || ( \
	    tmux new-session -s "sal/work" -n main -d \
	    && tmux new-window -n gcc/debug -c $${PWD}/../build/gcc-debug \
	    && tmux new-window -n gcc/release -c $${PWD}/../build/gcc-release \
	    && tmux new-window -n clang/debug -c $${PWD}/../build/clang-debug \
	    && tmux new-window -n clang/release -c $${PWD}/../build/clang-release \
	    && tmux new-window -n infra -c $${PWD}/../build/infra \
	    && tmux select-window -t "sal/work:main" \
	  )
	tmux attach-session -t "sal/work"


# Generic editing target {{{1
#


.edit:
	$(EDITOR) $(EDITORFLAGS) $(edit)/list.cmake $$(grep "$(edit)/" $(edit)/list.cmake)

tags:
	cscope -bkR -I . -I ../build/clang-debug -f ../cscope.out
	ctags -R -f ../tags --excmd=pattern --file-scope=no --if0 --tag-relative


# Building targets {{{1
#


.cmake:
	printf "\033[1;36m$(build)\033[m\n"
	cmake --build ../build/$(build)

gcc-debug:
	$(MAKE) .cmake build=$@
.build:: gcc-debug

gcc-release:
	$(MAKE) .cmake build=$@
.build:: gcc-release

clang-debug:
	$(MAKE) .cmake build=$@
.build:: clang-debug

clang-release:
	$(MAKE) .cmake build=$@
.build:: clang-release


# Testing targets {{{1
#


.ctest:
	printf "\033[1;36m$(build)\033[m\n"
	cd ../build/$(build) && ctest --output-on-failure

gcc-debug-test:
	$(MAKE) .ctest build=gcc-debug
test:: gcc-debug-test

gcc-release-test:
	$(MAKE) .ctest build=gcc-release
test:: gcc-release-test

clang-debug-test:
	$(MAKE) .ctest build=clang-debug
test:: clang-debug-test

clang-release-test:
	$(MAKE) .ctest build=clang-release
test:: clang-release-test


# Buildsystem(s) initialisation {{{1
#


.init_cmake:
	printf "\033[1;36m$(config)\033[m\n"
	mkdir -p ../build/$(config) \
	  && cd ../build/$(config) \
	  && cmake ../../sal -G Ninja $(CMAKE_OPTS) -DSAL_BENCH=yes -DCMAKE_BUILD_TYPE=$(config.type)

.init_cmake_gcc_debug:
	$(MAKE) .init_cmake config=gcc-debug config.type=Debug CXX=g++-6 CC=gcc-6
init:: .init_cmake_gcc_debug

.init_cmake_gcc_release:
	$(MAKE) .init_cmake config=gcc-release config.type=Release CXX=g++-6 CC=gcc-6
init:: .init_cmake_gcc_release

.init_cmake_clang_debug:
	$(MAKE) .init_cmake config=clang-debug config.type=Debug CXX=clang++ CC=clang
init:: .init_cmake_clang_debug

.init_cmake_clang_release:
	$(MAKE) .init_cmake config=clang-release config.type=Release CXX=clang++ CC=clang
init:: .init_cmake_clang_release

.init_cmake_infra:
	$(MAKE) .init_cmake config=infra config.type=Coverage CXX=g++-6 CC=gcc-6 GCOV=gcov-6 CMAKE_OPTS="-DSAL_DOCS=yes"
init:: .init_cmake_infra


.SILENT:
