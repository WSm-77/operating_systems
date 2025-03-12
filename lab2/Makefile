CC=gcc
SRC_DIR=collatz_lib/src
INCLUDE_DIR=collatz_lib/include
BUILD_DIR=collatz_lib/build
OBJ_SUBDIR=obj
LIB_SUBDIR=lib

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		echo "cleaning..."; \
		rm -r $(BUILD_DIR); \
	else \
		echo "nothing to clean"; \
	fi

$(BUILD_DIR)/$(OBJ_SUBDIR)/%.o: $(SRC_DIR)/%.c
	@if ! [ -d $(BUILD_DIR)/$(OBJ_SUBDIR) ]; then \
		mkdir -p $(BUILD_DIR)/$(OBJ_SUBDIR); \
	fi

	gcc -c -I$(INCLUDE_DIR) $< -o $@
