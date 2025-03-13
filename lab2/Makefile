.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS = -x -c

# sources
SRCS=$(wildcard $(SRC_DIR)/*.c)

# library
SRC_DIR=collatz_lib/src
INCLUDE_DIR=collatz_lib/include
LIB_BUILD_DIR=collatz_lib/build
OBJ_SUBDIR=obj
LIB_SUBDIR=lib
STATIC_LIB_SUBDIR=static
SHARED_LIB_SUBDIR=shared

OBJS_SHARED=$(patsubst $(SRC_DIR)/%.c,$(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(OBJ_SUBDIR)/%.o,$(SRCS))

# app
APP_DIR=app
MAIN_DIR=$(APP_DIR)/main
APP_BUILD_DIR=$(APP_DIR)/build

# compiler
CC=gcc
C_STATIC_FLAGS=-c -I$(INCLUDE_DIR)
C_SHARED_FLAGS=-c -I$(INCLUDE_DIR) -fPIC

all: static_app shared_app

clean:
	@flag=0; \
	if [ -d $(LIB_BUILD_DIR) ]; then \
		echo "cleaning $(LIB_BUILD_DIR)..."; \
		rm -r $(LIB_BUILD_DIR); \
		flag=1; \
	fi; \
	if [ -d $(APP_BUILD_DIR) ]; then \
		echo "cleaning $(APP_BUILD_DIR)..."; \
		rm -r $(APP_BUILD_DIR); \
		flag=1; \
	fi; \
	if [ "$$flag" -eq 0 ]; then \
		echo "nothing to clean"; \
	fi

##################
# shared library #
##################

$(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(OBJ_SUBDIR)/%.o: $(SRC_DIR)/%.c
	@if ! [ -d $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(OBJ_SUBDIR) ]; then \
		mkdir -p $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(OBJ_SUBDIR); \
	fi

	$(CC) $(C_STATIC_FLAGS) $< -o $@

$(APP_BUILD_DIR)/main_static.o: $(MAIN_DIR)/main.c
	@if ! [ -d $(APP_BUILD_DIR) ]; then \
		mkdir -p  $(APP_BUILD_DIR); \
	fi

	$(CC) $(C_STATIC_FLAGS) $< -o $@

libcollatz_static: $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(OBJ_SUBDIR)/collatz.o $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(OBJ_SUBDIR)/test_collatz.o
	@if ! [ -d $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(LIB_SUBDIR) ]; then \
		mkdir -p $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(LIB_SUBDIR); \
	fi

	ar rsc $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(LIB_SUBDIR)/libcollatz_static.a $^

static_app: libcollatz_static $(APP_BUILD_DIR)/main_static.o
	@# first option
	@# $(CC) -I$(INCLUDE_DIR) $(APP_BUILD_DIR)/main_static.o $(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(LIB_SUBDIR)/libcollatz_static.a -o $(APP_BUILD_DIR)/main_static

	@# second option
	$(CC) -I$(INCLUDE_DIR) $(APP_BUILD_DIR)/main_static.o -L$(LIB_BUILD_DIR)/$(STATIC_LIB_SUBDIR)/$(LIB_SUBDIR) -lcollatz_static -o $(APP_BUILD_DIR)/main_static

##################
# shared library #
##################

$(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(OBJ_SUBDIR)/%.o: $(SRC_DIR)/%.c
	@if ! [ -d $(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(OBJ_SUBDIR) ]; then \
		mkdir -p $(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(OBJ_SUBDIR); \
	fi

	$(CC) $(C_SHARED_FLAGS) $< -o $@

$(APP_BUILD_DIR)/main_shared.o: $(MAIN_DIR)/main.c
	@if ! [ -d $(APP_BUILD_DIR) ]; then \
		mkdir -p  $(APP_BUILD_DIR); \
	fi

	$(CC) $(C_SHARED_FLAGS) $< -o $@

libcollatz_shared: $(OBJS_SHARED)
	@if ! [ -d $(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(LIB_SUBDIR) ]; then \
		mkdir -p $(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(LIB_SUBDIR); \
	fi

	@path_to_library=$(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(LIB_SUBDIR);
	@full_path_to_library=$$(realpath $$path_to_library);

	$(CC) -shared -o $$path_to_library/libcollatz_shared.so.1.0 -Wl,-soname,libcollatz_shared.so.1 $^

	ln -sf $$full_path_to_library/libcollatz_shared.so.1.0 $$full_path_to_library/libcollatz_shared.so.1
	ln -sf $$full_path_to_library/libcollatz_shared.so.1 $$full_path_to_library/libcollatz_shared.so

shared_app: libcollatz_shared $(APP_BUILD_DIR)/main_shared.o
	$(CC) -I$(INCLUDE_DIR) $(APP_BUILD_DIR)/main_shared.o -L$(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(LIB_SUBDIR) -lcollatz_shared -o $(APP_BUILD_DIR)/main_shared -Wl,-rpath=$(LIB_BUILD_DIR)/$(SHARED_LIB_SUBDIR)/$(LIB_SUBDIR)

.PHONY: clean all static_app shared_app
