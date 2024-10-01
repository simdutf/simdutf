# ideally I would like to upload this to the simdutf package registry but
# I think my privs need to be higher for that to work.

# clang format 18
.PHONY: build-18 publish-18
build-18:
	docker build --build-arg CLANGVERSION=18 --tag ghcr.io/pauldreik/clang-format-18:latest .
publish-18: build-18
	docker push ghcr.io/pauldreik/clang-format-18:latest
