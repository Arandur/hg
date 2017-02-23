BIN_NAME = hg
SRC_EXT = cpp
SRC_PATH = src
COMPILE_FLAGS = -std=c++14 -Wall -Wextra -pedantic
RCOMPILE_FLAGS = -D NDEBUG -O3
DCOMPILE_FLAGS = -g -D DEBUG
INCLUDES = -I $(SRC_PATH)
LINK_FLAGS = 
RLINK_FLAGS =
DLINK_FLAGS =

SHELL = /bin/bash

# Verbose option, to output compile and link commands
export V = false
export CMD_PREFIX = @
ifeq ($(V),true)
	CMD_PREFIX =
endif

# Combine compiler and linker flags
release: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

# OS detection
ifeq ($(OS),Windows_NT)
	CXXFLAGS += -D WIN32
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
		CXXFLAGS += -D AMD64
	else
		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			CXXFLAGS += -D AMD64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			CXXFLAGS += -D IA32
		endif
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CXXFLAGS += -D LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
		CXXFLAGS += -D OSX
	endif
	UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		CXXFLAGS += -D AMD64
	endif
	ifneq ($(filter %86,$(UNAME_P)),)
		CXXFLAGS += -D IA32
	endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		CXXFLAGS += -D ARM
	endif
endif

release: export BUILD_PATH := build/release
release: export BIN_PATH := bin/release
debug: export BUILD_PATH := build/debug
debug: export BIN_PATH := bin/debug

SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)')
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPS = $(OBJECTS:.o=.d)

TIME_FILE = $(dir $@).$(notdir $@)_time
START_TIME = date '+%s' > $(TIME_FILE)

# Don't know how to do this on Windows at all... D:
ifeq ($(UNAME_S),Linux)
	END_TIME = read st < $(TIME_FILE) ; \
						 $(RM) $(TIME_FILE) ; \
						 st=$$((`date '+%s'` - $$st - 86400)) ; \
						 echo `date -u -d @$$st '+%H:%M:%S'`
endif
ifeq ($(UNAME_S),Darwin)
	END_TIME = read st < $(TIME_FILE) ; \
						 $(RM) $(TIME_FILE) ; \
						 st=$$((`date '+%s'` - $$st)) ; \
						 h=$$(($$st / 3600)) ; \
						 m=$$(($$st / 60 % 60)) ; \
						 s=$$(($$st % 60)) ; \
						 printf '%02d:%02d:%02d\n' $$h $$m $$s
endif

# Versioning
VERSION := $(shell git describe --tags --long --dirty --always | \
	sed 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)-\?.*-\([0-9]*\)-\(.*\)/\1 \2 \3 \4 \5/g')
VERSION_MAJOR := $(word 1, $(VERSION))
VERSION_MINOR := $(word 2, $(VERSION))
VERSION_PATCH := $(word 3, $(VERSION))
VERSION_REVISION := $(word 4, $(VERSION))
VERSION_HASH := $(word 5, $(VERSION))
VERSION_STRING := \
	"$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_REVISION)"
override CXXFLAGS := $(CXXFLAGS) \
	-D VERSION_MAJOR=$(VERSION_MAJOR) \
	-D VERSION_MINOR=$(VERSION_MINOR) \
	-D VERSION_PATCH=$(VERSION_PATCH) \
	-D VERSION_REVISION=$(VERSION_REVISION) \
	-D VERSION_HASH=\"$(VERSION_HASH)\"

.PHONY: debug
debug: dirs
	@echo "Beginning debug build"
	@$(START_TIME)
	@$(MAKE) all --no-print-directory
	@echo -n "Total build time: "
	@$(END_TIME)

.PHONY: release
release: dirs
	@echo "Beginning release build"
	@$(START_TIME)
	@$(MAKE) all --no-print-directory
	@echo -n "Total build time: "
	@$(END_TIME)

.PHONY: dirs
dirs:
	@echo "Creating directories"
	@echo "SOURCES:" $(SOURCES)
	mkdir -p $(dir $(OBJECTS))
	mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@echo "Deleting directories"
	@$(RM) -r build
	@$(RM) -r bin
	@$(RM) $(BIN_NAME)
	@$(RM) $(SOURCES:.$(SRC_EXT)=.$(SRC_EXT).orig)

.PHONY: lint
lint:
	@astyle -A1W1k3t2UHNYLwpxyxC80 `find $(SRC_PATH) -name "*.c" -o -name "*.h"`

all: $(BIN_PATH)/$(BIN_NAME)
	@ln -sf $< $(BIN_NAME)

$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	@echo "Linking: $@"
	@$(START_TIME)
	$(CMD_PREFIX)$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	@echo -en "\t Link time: "
	@$(END_TIME)

-include $(DEPS)

$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	@$(START_TIME)
	$(CMD_PREFIX)$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@
	@echo -en "\t Compile time: "
	@$(END_TIME)
