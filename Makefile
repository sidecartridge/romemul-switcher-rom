.PHONY: st amiga debug test clean format format-check tidy check

# Version from file
VERSION := $(shell cat version.txt)

DEBUG ?= 0
TEST ?= 0
STARTUP_ROM_ASM ?= startup_ste.s

NEED_ROM_BASE_ADDR := 0
ifeq ($(strip $(MAKECMDGOALS)),)
NEED_ROM_BASE_ADDR := 1
endif
ifneq ($(filter st debug test,$(MAKECMDGOALS)),)
NEED_ROM_BASE_ADDR := 1
endif
ifeq ($(NEED_ROM_BASE_ADDR),1)
ifndef ROM_BASE_ADDR_UL
$(error ROM_BASE_ADDR_UL is required. Use ./build.sh [st|ste] or pass ROM_BASE_ADDR_UL=0x00E00000UL)
endif
endif

st:
	$(MAKE) -C src/st DEBUG=$(DEBUG) TEST=$(TEST) ROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_UL) STARTUP_ROM_ASM=$(STARTUP_ROM_ASM)

amiga:
	$(MAKE) -C src/amiga DEBUG=$(DEBUG) TEST=$(TEST) ROM_BASE_ADDR_UL=$(ROM_BASE_ADDR_UL)

debug:
	$(MAKE) st DEBUG=1

test:
	$(MAKE) st TEST=1

format:
	./scripts/clang-checks.sh format

format-check:
	./scripts/clang-checks.sh format-check

tidy:
	./scripts/clang-checks.sh tidy

check:
	./scripts/clang-checks.sh check

clean:
	$(MAKE) -C src/st clean
	$(MAKE) -C src/amiga clean

## Tag this version
.PHONY: tag
tag:
	git tag $(VERSION) && git push origin $(VERSION) && \
	echo "Tagged: $(VERSION)"
