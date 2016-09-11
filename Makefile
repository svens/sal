project = sal


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


# tmux "IDE" {{{1
#

ide:
	tmux has-session -t "$(project)" >/dev/null 2>&1 \
	  && ( tmux detach-client -s "$(project)" || true ) \
	  || ( tmux new-session -s "$(project)" -n main -d \; \
	    split-window \; \
	    select-layout main-vertical \; \
	    select-pane -L \; \
	    new-window -n root \; \
	    new-window -n gcc-debug -c .work/gcc-debug \; \
	    new-window -n gcc-release -c .work/gcc-release \; \
	    new-window -n clang-debug -c .work/clang-debug \; \
	    new-window -n clang-release -c .work/clang-release \; \
	    new-window -n infra -c .work/infra \; \
	    select-window -t "$(project):main" \; \
	    bind-key . choose-window 'swap-pane -t main.1 -s 1.0; swap-window -d -t 1 -s %%; swap-pane -t main.1 -s 1.0; select-window -t main.1' \
	  )
	tmux attach-session -t "$(project)"


# Generic editing target {{{1
#


.edit:
	$(EDITOR) $(EDITORFLAGS) $(edit)/list.cmake $$(grep "^[[:space:]]*$(edit)/" $(edit)/list.cmake)

tags:
	find sal bench -name '*pp' > .work/sources.txt
	cscope -bckR -I. -I.work/clang-debug/sal -f.work/cscope.out -i.work/sources.txt
	ctags -R -f .work/tags --excmd=pattern --file-scope=no --if0 --tag-relative -L.work/sources.txt


# Building targets {{{1
#


.cmake:
	printf "\033[1;36m$(build)\033[m\n"
	cmake --build .work/$(build)

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
	cd .work/$(build) && ctest --output-on-failure

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


# Coverage target {{{1
#

gen-cov:
	ninja -C .work/infra gen-cov
	open .work/infra/cov/index.html


# Documentation target {{{1
#

gen-doc:
	ninja -C .work/infra gen-doc
	open .work/infra/docs/index.html


# Buildsystem(s) initialisation {{{1
#


.init_cmake:
	printf "\033[1;36m$(config)\033[m\n"
	mkdir -p .work/$(config) \
	  && cd .work/$(config) \
	  && cmake ../.. -G Ninja $(CMAKE_OPTS) -DSAL_BENCH=yes -DCMAKE_BUILD_TYPE=$(config.type)

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
