# Front door for the containerized backend contract tiers. These targets just
# delegate to ./test (the dispatcher); see docker/mdsplus/ and docker/uda/ for
# what each leg builds. The main build stays CMake (`cmake -B build && ...`).
.PHONY: help test test-common test-mdsplus test-uda

help:
	@echo "make test-common    always-on backends, host-native     (== ./test common)"
	@echo "make test-mdsplus   MDSplus contract tier in Docker      (== ./test mdsplus)"
	@echo "make test-uda       UDA contract tier in Docker          (== ./test uda)"
	@echo "make test           common + mdsplus + uda               (== ./test all)"
	@echo ""
	@echo "Override the platform on x86: PLATFORM=linux/amd64 make test-uda"

test-common:
	@./test common

test-mdsplus:
	@./test mdsplus

test-uda:
	@./test uda

test:
	@./test all
