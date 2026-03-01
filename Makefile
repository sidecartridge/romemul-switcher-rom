STCMD ?= stcmd
STCMD_ENV ?= STCMD_QUIET=1

.PHONY: st clean

st:
	$(STCMD_ENV) $(STCMD) make -C src/st

clean:
	$(STCMD_ENV) $(STCMD) make -C src/st clean
