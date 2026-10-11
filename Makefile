# unixlib (header-only)
#
# On Windows with TDM-GCC:
#   mingw32-make              -> builds tests and examples into build/
#   mingw32-make test         -> runs build/test_all.exe and build/test_ipc.exe
#
# Cross-compiling from macOS / Linux to Windows (macOS: brew install mingw-w64):
#   make cross                -> build/win64/*.exe  (x86_64-w64-mingw32-gcc)
#   make cross32              -> build/win32/*.exe  (i686-w64-mingw32-gcc)
#   make cross-all            -> both
#   make cross-zip            -> dist/unixlib-windows.zip (exes + README) to copy to Windows
#   make cross-test           -> runs the .exe files under Wine, if installed
#   (another compiler: make cross CC64=/path/to/x86_64-w64-mingw32-gcc)

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=gnu11
CFLAGS  += -Iinclude -Icompat
BUILD   ?= build

CC64    ?= x86_64-w64-mingw32-gcc
CC32    ?= i686-w64-mingw32-gcc
WINE    ?= wine

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

TUT  := $(patsubst tests/tutorial/%.c,$(BUILD)/tut/%$(EXE),$(wildcard tests/tutorial/*.c))
BINS := $(TUT) $(BUILD)/test_all$(EXE) $(BUILD)/test_ipc$(EXE) $(BUILD)/test_multi$(EXE) \
        $(BUILD)/pipe_fork$(EXE) $(BUILD)/sigint$(EXE)

all: $(BINS)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/test_all$(EXE): tests/test_all.c include/unixlib/*.h | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILD)/test_ipc$(EXE): tests/test_ipc.c include/unixlib/*.h compat/sys/*.h | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Two translation units: checks that the shared state
# (UNIXLIB_SHARED) links without duplicate symbols.
$(BUILD)/test_multi$(EXE): tests/test_multi_a.c tests/test_multi_b.c include/unixlib/*.h | $(BUILD)
	$(CC) $(CFLAGS) tests/test_multi_a.c tests/test_multi_b.c -o $@ $(LDFLAGS)

# Tutorial code, unmodified: checks that it compiles with -Icompat
$(BUILD)/tut/%$(EXE): tests/tutorial/%.c include/unixlib/*.h compat/*.h compat/sys/*.h | $(BUILD)
	mkdir -p $(BUILD)/tut
	$(CC) $(CFLAGS) -w $< -o $@ $(LDFLAGS)

$(BUILD)/%$(EXE): examples/%.c include/unixlib/*.h | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test: $(BUILD)/test_all$(EXE) $(BUILD)/test_ipc$(EXE) $(BUILD)/test_multi$(EXE)
	./$(BUILD)/test_all$(EXE)
	./$(BUILD)/test_ipc$(EXE)
	./$(BUILD)/test_multi$(EXE)

# ---- cross-compilation to Windows (static .exe executables) ----
CROSS_LDFLAGS := -static

cross-check64:
	@command -v $(firstword $(CC64)) >/dev/null 2>&1 || { \
	  echo "'$(firstword $(CC64))' not found. On macOS: brew install mingw-w64"; exit 1; }

cross-check32:
	@command -v $(firstword $(CC32)) >/dev/null 2>&1 || { \
	  echo "'$(firstword $(CC32))' not found. On macOS: brew install mingw-w64"; exit 1; }

cross: cross-check64
	$(MAKE) all CC="$(CC64)" EXE=.exe BUILD=build/win64 LDFLAGS="$(CROSS_LDFLAGS)"

cross32: cross-check32
	$(MAKE) all CC="$(CC32)" EXE=.exe BUILD=build/win32 LDFLAGS="$(CROSS_LDFLAGS)"

cross-all: cross cross32

cross-zip: cross-all
	mkdir -p dist
	rm -f dist/unixlib-windows.zip
	zip -qr dist/unixlib-windows.zip build/win64 build/win32 README.md

# Wine is optional (not available on every Mac, e.g. Apple Silicon without Rosetta).
cross-test: cross
	@command -v $(WINE) >/dev/null 2>&1 || { \
	  echo "Wine is not installed: copy build/win64/*.exe to Windows and run them there."; exit 1; }
	$(WINE) build/win64/test_all.exe
	$(WINE) build/win64/test_ipc.exe
	$(WINE) build/win64/test_multi.exe

clean:
	rm -rf build dist

.PHONY: all test clean cross cross32 cross-all cross-zip cross-test cross-check64 cross-check32
