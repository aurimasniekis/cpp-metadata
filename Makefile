# Convenience wrapper around CMake/CTest workflows for metadata.
# Real build system is CMake — this just memorizes common invocations.

BUILD_DIR        ?= build
BUILD_TYPE       ?= Debug
JOBS             ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
GENERATOR        ?=
CMAKE            ?= cmake
CTEST            ?= ctest
LLVM_PROFDATA    ?= xcrun llvm-profdata
LLVM_COV         ?= xcrun llvm-cov

CMAKE_GEN_FLAG   := $(if $(GENERATOR),-G "$(GENERATOR)",)

.DEFAULT_GOAL := help

.PHONY: help
help:
	@echo "metadata — make targets"
	@echo ""
	@echo "  make configure         Configure $(BUILD_DIR)/ ($(BUILD_TYPE))"
	@echo "  make build             Build everything in $(BUILD_DIR)/"
	@echo "  make test              Run ctest in $(BUILD_DIR)/"
	@echo "  make example           Run the metadata_basic example"
	@echo "  make examples          Build and run every metadata_* example (fails on any non-zero exit)"
	@echo "  make all               configure + build + test"
	@echo ""
	@echo "  make sanitize          Configure+build+test in build-san/ with ASan+UBSan"
	@echo "  make tidy              Configure+build in build-tidy/ with clang-tidy"
	@echo "  make release           Configure+build in build-release/ (Release)"
	@echo "  make coverage          Configure+build+test in build-coverage/ with Clang coverage"
	@echo "  make docs              Configure+build Doxygen HTML in build-docs/"
	@echo "  make no-json           Configure+build+test in build-no-json/ with METADATA_WITH_NLOHMANN_JSON=OFF"
	@echo ""
	@echo "  make format            Run clang-format -i over project sources"
	@echo "  make format-check      Verify formatting without writing"
	@echo ""
	@echo "  make ci                Run the full pre-push gate: format-check + tidy + test + sanitize + release + no-json"
	@echo ""
	@echo "  make clean             Remove $(BUILD_DIR)/"
	@echo "  make distclean         Remove all build-* directories"
	@echo ""
	@echo "Variables: BUILD_DIR=$(BUILD_DIR) BUILD_TYPE=$(BUILD_TYPE) JOBS=$(JOBS)"

.PHONY: configure
configure:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_GEN_FLAG) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

.PHONY: build
build: configure
	$(CMAKE) --build $(BUILD_DIR) -j $(JOBS)

.PHONY: test
test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

.PHONY: example
example: build
	$(BUILD_DIR)/examples/metadata_basic

.PHONY: examples
examples: build
	@set -e; \
	failures=""; \
	for ex in $(BUILD_DIR)/examples/metadata_*; do \
		if [ -x "$$ex" ] && [ ! -d "$$ex" ]; then \
			name=$$(basename "$$ex"); \
			echo "=== $$name ==="; \
			if ! "$$ex" >/dev/null; then \
				echo "FAIL: $$name (exit $$?)"; \
				failures="$$failures $$name"; \
			fi; \
		fi; \
	done; \
	if [ -n "$$failures" ]; then \
		echo "Examples that failed:$$failures"; \
		exit 1; \
	fi

.PHONY: all
all: test

.PHONY: ci
ci: format-check tidy test sanitize release no-json
	@echo ""
	@echo "ci: all checks passed"

.PHONY: sanitize
sanitize:
	$(CMAKE) -S . -B build-san $(CMAKE_GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug -DMETADATA_ENABLE_SANITIZERS=ON
	$(CMAKE) --build build-san -j $(JOBS)
	$(CTEST) --test-dir build-san --output-on-failure

.PHONY: tidy
tidy:
	$(CMAKE) -S . -B build-tidy $(CMAKE_GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug -DMETADATA_ENABLE_CLANG_TIDY=ON
	$(CMAKE) --build build-tidy -j $(JOBS)

.PHONY: release
release:
	$(CMAKE) -S . -B build-release $(CMAKE_GEN_FLAG) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build build-release -j $(JOBS)
	$(CTEST) --test-dir build-release --output-on-failure

.PHONY: no-json
no-json:
	$(CMAKE) -S . -B build-no-json $(CMAKE_GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug -DMETADATA_WITH_NLOHMANN_JSON=OFF
	$(CMAKE) --build build-no-json -j $(JOBS)
	$(CTEST) --test-dir build-no-json --output-on-failure

.PHONY: coverage
coverage:
	$(CMAKE) -S . -B build-coverage $(CMAKE_GEN_FLAG) \
		-DCMAKE_BUILD_TYPE=Debug -DMETADATA_ENABLE_COVERAGE=ON
	$(CMAKE) --build build-coverage -j $(JOBS)
	rm -f build-coverage/*.profraw build-coverage/metadata.profdata
	LLVM_PROFILE_FILE="$(CURDIR)/build-coverage/metadata-%p.profraw" \
		$(CTEST) --test-dir build-coverage --output-on-failure
	$(LLVM_PROFDATA) merge -sparse build-coverage/*.profraw \
		-o build-coverage/metadata.profdata
	$(LLVM_COV) report build-coverage/tests/metadata_tests \
		-instr-profile=build-coverage/metadata.profdata \
		-ignore-filename-regex='(_deps|tests)/'
	$(LLVM_COV) show build-coverage/tests/metadata_tests \
		-instr-profile=build-coverage/metadata.profdata \
		-ignore-filename-regex='(_deps|tests)/' \
		-format=html -output-dir=build-coverage/coverage-html \
		-show-line-counts-or-regions
	@echo "HTML report: build-coverage/coverage-html/index.html"

.PHONY: docs
docs:
	$(CMAKE) -S . -B build-docs $(CMAKE_GEN_FLAG) \
		-DMETADATA_BUILD_DOCS=ON \
		-DMETADATA_WITH_NLOHMANN_JSON=ON \
		-DMETADATA_WITH_PARCEL=ON
	$(CMAKE) --build build-docs --target metadata_docs -j $(JOBS)
	@echo "HTML report: build-docs/docs/html/index.html"

FORMAT_FILES := $(shell find include tests examples -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.cc' \) 2>/dev/null)

.PHONY: format
format:
	@if [ -z "$(FORMAT_FILES)" ]; then echo "no source files found"; else clang-format -i $(FORMAT_FILES); fi

.PHONY: format-check
format-check:
	@if [ -z "$(FORMAT_FILES)" ]; then echo "no source files found"; else clang-format --dry-run --Werror $(FORMAT_FILES); fi

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: distclean
distclean:
	rm -rf build build-san build-tidy build-release build-coverage build-docs build-no-json cmake-build-*
