.PHONY: st debug test clean format format-check tidy check

DEBUG ?= 0
TEST ?= 0

st:
	make -C src/st DEBUG=$(DEBUG) TEST=$(TEST)

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
	make -C src/st clean
