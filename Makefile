.PHONY: st clean format format-check tidy check

st:
	make -C src/st

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
