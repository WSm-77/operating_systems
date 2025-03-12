# library
SRC_DIR=collatz_lib/src
INCLUDE_DIR=collatz_lib/include
LIB_BUILD_DIR=collatz_lib/build
OBJ_SUBDIR=obj
LIB_SUBDIR=lib
STATIC_LIB_SUBDIR=static

# app
APP_DIR=app
MAIN_DIR=$(APP_DIR)/main
APP_BUILD_DIR=$(APP_DIR)/build

# compiler
CC=gcc
C_STATIC_FLAGS=-c -I$(INCLUDE_DIR)

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
