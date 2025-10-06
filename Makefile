VERSION ?= v0.1.0
BRANCH_DEV = dev
BRANCH_MAIN = main

.PHONY: help release commit push merge tag test changelog

help:
	@echo "Available commands:"
	@echo "  make commit                  - Add and commit all files (on $(BRANCH_DEV) branch)"
	@echo "  make push                    - Push the $(BRANCH_DEV) branch"
	@echo "  make merge                   - Merge $(BRANCH_DEV) into $(BRANCH_MAIN)"
	@echo "  make tag VERSION=vX.Y.Z     - Create and push a Git tag (default: $(VERSION))"
	@echo "  make release VERSION=vX.Y.Z - Full release: changelog + commit + push + merge + tag"
	@echo "  make test                    - Compile and run tests"
	@echo "  make changelog               - Update CHANGELOG.md using script"

commit:
	git checkout $(BRANCH_DEV)
	@if [ -n "$$(git status --porcelain)" ]; then \
		echo "📝 Committing changes..."; \
		git add .; \
		git commit -m "chore(release): prepare $(VERSION)"; \
	else \
		echo "✅ Nothing to commit."; \
	fi

push:
	git push origin $(BRANCH_DEV)

merge:
	git checkout $(BRANCH_MAIN)
	git merge --no-ff --no-edit $(BRANCH_DEV)
	git push origin $(BRANCH_MAIN)

tag:
	@if git rev-parse $(VERSION) >/dev/null 2>&1; then \
		echo "❌ Tag $(VERSION) already exists."; \
		exit 1; \
	else \
		echo "🏷️  Creating annotated tag $(VERSION)..."; \
		git tag -a $(VERSION) -m "Release version $(VERSION)"; \
		git push origin $(VERSION); \
	fi

release:
	make changelog
	make commit
	make push
	make merge
	make tag VERSION=$(VERSION)

test:
	cd build && ctest --output-on-failure

changelog:
	bash scripts/update_changelog.sh
