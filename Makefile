sal :=


ifeq ($(sal),)
all:: build
else
all:: .sal
endif


ROOT := ../..
CMAKE_OPTS := $(ROOT) -G Ninja -DSAL_UNITTESTS=yes

EDITOR ?= echo
EDITORFLAGS ?=


#
# tmux
#

edit:
	tmux has-session -t "sal/edit" >/dev/null 2>&1 \
	    && tmux detach-client -s "sal/edit" \
	    || ( \
	        tmux new-session -s "sal/edit" -n main -d \
	        && tmux new-window -n edit \
	        && tmux new-window -n bench \
	        && tmux select-window -t "sal/edit:edit" \
	    )
	tmux attach-session -t "sal/edit"

work:
	tmux has-session -t "sal/work" >/dev/null 2>&1 \
	    && tmux detach-client -s "sal/work" \
	    || ( \
	        tmux new-session -s "sal/work" -n main -d \
	        && tmux new-window -n work \
	        && tmux new-window -n bench \
	        && tmux new-window -n gcc/debug -c $${PWD}/build/gcc-debug \
	        && tmux new-window -n gcc/release -c $${PWD}/build/gcc-release \
	        && tmux new-window -n clang/debug -c $${PWD}/build/clang-debug \
	        && tmux new-window -n clang/release -c $${PWD}/build/clang-release \
	        && tmux new-window -n infra -c $${PWD}/build/infra \
	        && tmux select-window -t "sal/work:work" \
	    )
	tmux attach-session -t "sal/work"


#
# edit
#

.sal:
	$(EDITOR) $(EDITORFLAGS) $(sal)/list.cmake $$(grep "$(sal)/" $(sal)/list.cmake)


#
# gen-cov & gen-doc
#

build/infra:
	mkdir -p $@ && cd $@ && \
	  CXX=g++-5 CC=gcc-5 GCOV=gcov-5 cmake $(CMAKE_OPTS) -DCMAKE_BUILD_TYPE=Coverage -DSAL_DOCS=yes
infra: build/infra


#
# cmake
#

.cmake:
	printf "\033[1;36m$(target)\033[m\n"
	mkdir -p build/$(target) \
	  && cd build/$(target) \
	  && cmake $(CMAKE_OPTS) -DSAL_BENCH=yes -DCMAKE_BUILD_TYPE=$(target.type)

cmake/gcc-debug:
	$(MAKE) .cmake target=gcc-debug target.type=Debug CXX=g++-5 CC=gcc-5
cmake:: cmake/gcc-debug

cmake/gcc-release:
	$(MAKE) .cmake target=gcc-release target.type=Release CXX=g++-5 CC=gcc-5
cmake:: cmake/gcc-release

cmake/clang-debug:
	$(MAKE) .cmake target=clang-debug target.type=Debug CXX=clang++ CC=clang
cmake:: cmake/clang-debug

cmake/clang-release:
	$(MAKE) .cmake target=clang-release target.type=Release CXX=clang++ CC=clang
cmake:: cmake/clang-release


#
# build
#

.build:
	printf "\033[1;36m$(target)\033[m\n"
	cmake --build build/$(target)

gcc-debug:
	$(MAKE) .build target=gcc-debug
build:: gcc-debug

gcc-release:
	$(MAKE) .build target=gcc-release
build:: gcc-release

clang-debug:
	$(MAKE) .build target=clang-debug
build:: clang-debug

clang-release:
	$(MAKE) .build target=clang-release
build:: clang-release


#
# test
#

.test:
	printf "\033[1;36m$(target)\033[m\n"
	cmake --build build/$(target) --target test

test/gcc-debug:
	$(MAKE) .test target=gcc-debug
test:: test/gcc-debug

test/gcc-release:
	$(MAKE) .test target=gcc-release
test:: test/gcc-release

test/clang-debug:
	$(MAKE) .test target=clang-debug
test:: test/clang-debug

test/clang-release:
	$(MAKE) .test target=clang-release
test:: test/clang-release


.SILENT:
