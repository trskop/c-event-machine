LOCAL_PWD := $(shell pwd)

SRC_DIR ?= src
SRC = $(SRC_DIR)/

OUT_DIR ?= out
CC_VERSION := $(shell $(CC) -dumpversion)
MACHINE_INFO := $(shell $(CC) -dumpmachine)
OUT = $(OUT_DIR)/$(CC)-$(CC_VERSION)-$(MACHINE_INFO)/
DEPS = $(OUT)dependency-files/

OS := $(shell uname -s)

LIB_DIR = lib
LIB_BASE_NAME = event-machine
LIB = $(OUT)$(LIB_DIR)/
LIB_PREFIX = $(LIB)lib
SO_TARGET = $(LIB_PREFIX)$(LIB_BASE_NAME).so
A_TARGET = $(LIB_PREFIX)$(LIB_BASE_NAME).a

INSTALL_DIR ?= install-output

EXE_DIR = $(OUT)exe
EXE = $(EXE_DIR)/

EXAMPLE_DIR ?= example
EXAMPLE = $(EXAMPLE_DIR)/

ifeq ($(OS),Darwin)
# Currently timers aren't supported.
SOURCES := $(SRC)/event-machine.c
else
SOURCES := $(shell find '$(SRC)' -name '*.c')
endif
OBJECTS = $(subst $(SRC),$(OUT),$(SOURCES:.c=.o))
DEPENDENCY_FILES = $(subst $(SRC),$(DEPS),$(SOURCES:.c=.deps))

EXAMPLE_SOURCES := $(shell find '$(EXAMPLE)' -name '*.c')
EXAMPLE_EXECUTABLES = $(subst $(EXAMPLE),$(EXE),$(EXAMPLE_SOURCES:.c=))

# {{{ Command and building flags ##############################################

INCLUDE_PATH = $(SRC_DIR)

CC_OUTPUT_OPTION = -o $@
CC ?= gcc
CFLAGS += -Wall -std=c11

ifeq ($(OS),Linux)
CFLAGS += -DUSE_EPOLL -DUSE_PIPE2
endif
ifeq ($(OS),Darwin)
CFLAGS += -DUSE_KQUEUE
endif

CFLAGS += -g
CPPFLAGS += $(addprefix -I,$(INCLUDE_PATH))
#LDFLAGS +=
#TARGET_ARCH +=
#LDLIBS +=

DOXYGEN = doxygen
DOXYGEN_CONFIG = tools/Doxyfile

MK_OUT_DIRS = \
    [ -e '$(dir $@)' ] || \
        { mkdir -p '$(dir $@)' && \
            echo '*** INFO: Creating directory: "$(dir $@)" ***'; }

# }}} Command and building flags ##############################################

# {{{ Generic building rules ##################################################

$(DEPS)%.deps: $(SRC)%.c
	@$(MK_OUT_DIRS)
	$(CC) $(CPPFLAGS) -MM -MT '$(subst $(SRC),$(OUT),$(<:%.c=%.o))' $(CC_OUTPUT_OPTION) $<

$(OUT)%.o: $(SRC)%.c
	@$(MK_OUT_DIRS)
	$(CC) $(CFLAGS) -fPIC $(CPPFLAGS) $(TARGET_ARCH) -c $(CC_OUTPUT_OPTION) $<

$(EXE)%: $(EXAMPLE)%.c
	@$(MK_OUT_DIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -L$(LIB) $(LDFLAGS) $(TARGET_ARCH) $< $(LOADLIBES) $(LDLIBS) -l$(LIB_BASE_NAME) $(CC_OUTPUT_OPTION)

# }}} Generic building rules ##################################################

all: build
.PHONY: all

build: build-so build-a
.PHONY: build

build-so: $(SO_TARGET)
.PHONY: build-so

$(SO_TARGET): $(OBJECTS)
	@$(MK_OUT_DIRS)
	$(CC) $(CFLAGS) -fPIC $(CPPFLAGS) $(LDFLAGS) -shared $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) $(CC_OUTPUT_OPTION)

build-a: $(A_TARGET)
.PHONY: build-a

$(A_TARGET): $(OBJECTS)
	@$(MK_OUT_DIRS)
	$(AR) $(ARFLAGS) $@ $^

examples: build-examples
.PHONY: examples

build-examples: $(EXAMPLE_EXECUTABLES)
.PHONY: build-examples

$(EXAMPLE_EXECUTABLES): $(SO_TARGET)
#$(EXAMPLE_EXECUTABLES): $(A_TARGET)

install: all
	install -d $(INSTALL_DIR)/bin/
	install -d $(INSTALL_DIR)/lib/
	install -d $(INSTALL_DIR)/include/
	install -d $(INSTALL_DIR)/include/event-machine/
	install -D $(SO_TARGET) $(INSTALL_DIR)/lib
	install -D $(A_TARGET) $(INSTALL_DIR)/lib
	install -D src/event-machine.h $(INSTALL_DIR)/include/
	install -D src/event-machine/result.h $(INSTALL_DIR)/include/event-machine
.PHONY: install

include-path:
	@echo $(addprefix $(LOCAL_PWD)/,$(INCLUDE_PATH))
.PHONY: include-path
NO_DEPS_TARGETS += include-path

doxygen:
	$(DOXYGEN) $(DOXYGEN_CONFIG)
.PHONY: doxygen
NO_DEPS_TARGETS += doxygen

api-doc: doxygen
.PHONY: api-doc
NO_DEPS_TARGETS += api-doc

clean: clean-objects clean-dependency-files
.PHONY: clean
NO_DEPS_TARGETS += clean

clean-objects:
	rm -f $(OBJECTS)
.PHONY: clean-objects
NO_DEPS_TARGETS += clean-objects

clean-dependency-files:
	rm -f $(DEPENDENCY_FILES)
.PHONY: clean-dependency-files
NO_DEPS_TARGETS += clean-dependency-files

distclean dist-clean:
	rm -fr $(OUT_DIR)
.PHONY: distclean dist-clean
NO_DEPS_TARGETS += distclean dist-clean

INCLUDE_DEPENDENCIES = Y

# If $(MAKECMDGOALS) is empty, then we are calling default target which
# requires dependency files to be included.
ifneq ($(MAKECMDGOALS),)
ifeq ($(filter-out $(NO_DEPS_TARGETS),$(MAKECMDGOALS)),)
INCLUDE_DEPENDENCIES = N
endif
endif

ifeq ($(INCLUDE_DEPENDENCIES),Y)
-include $(DEPENDENCY_FILES)
endif
