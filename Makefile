# -------- Release Makefile (robuste et reprenable) --------
VERSION ?= v0.1.0
BRANCH_DEV = dev
BRANCH_MAIN = main

# si VERSION ne commence pas par "v", on préfixe
# Ex: VERSION=1.9.0 -> TAG=v1.9.0 ; VERSION=v1.9.0 -> TAG=v1.9.0
TAG := $(if $(filter v%,$(VERSION)),$(VERSION),v$(VERSION))

.PHONY: help release release-fast commit push push-main merge tag resume-tag test changelog \
        submodules submodules-remote gh-release assert-dev assert-main assert-clean

help:
	@echo "Available commands:"
	@echo "  make release VERSION=vX.Y.Z    - Full release: changelog + commit(dev) + push(dev) + merge->main + push(main) + tag"
	@echo "  make release-fast VERSION=vX.Y.Z - Same as release but without changelog prompt (reprenable)"
	@echo "  make commit                     - Commit on $(BRANCH_DEV) (if there are changes)"
	@echo "  make push                       - Push $(BRANCH_DEV)"
	@echo "  make merge                      - Merge $(BRANCH_DEV) into $(BRANCH_MAIN) + push(main)"
	@echo "  make tag VERSION=vX.Y.Z         - Create and push annotated tag (safe if exists)"
	@echo "  make resume-tag VERSION=vX.Y.Z  - Only (re)push the tag step"
	@echo "  make gh-release VERSION=vX.Y.Z  - Create GitHub release from TAG (needs 'gh')"
	@echo "  make submodules                 - Sync/init submodules"
	@echo "  make test                       - Run ctest from ./build"
	@echo "Current VERSION=$(VERSION) -> TAG=$(TAG)"

# --- Guards ---
assert-dev:
	@if [ "$$(git rev-parse --abbrev-ref HEAD)" != "$(BRANCH_DEV)" ]; then \
		echo "ℹ️  Switching to $(BRANCH_DEV)"; \
		git checkout $(BRANCH_DEV); \
	fi

assert-main:
	@if [ "$$(git rev-parse --abbrev-ref HEAD)" != "$(BRANCH_MAIN)" ]; then \
		echo "ℹ️  Switching to $(BRANCH_MAIN)"; \
		git checkout $(BRANCH_MAIN); \
	fi

assert-clean:
	@if [ -n "$$(git status --porcelain)" ]; then \
		echo "❌ Working tree not clean. Commit or stash before release."; \
		exit 1; \
	fi

# --- Steps ---

commit: assert-dev
	@if [ -n "$$(git status --porcelain)" ]; then \
		echo "📝 Committing changes on $(BRANCH_DEV)..."; \
		git add .; \
		git commit -m "chore(release): prepare $(TAG)"; \
	else \
		echo "✅ Nothing to commit on $(BRANCH_DEV)."; \
	fi

push: assert-dev
	@echo "⬆️  Pushing $(BRANCH_DEV)..."
	@for i in 1 2 3; do \
		git push origin $(BRANCH_DEV) && s=0 && break || s=$$?; \
		echo "⚠️  Push $(BRANCH_DEV) failed (try $$i/3). Retrying in 2s..."; \
		sleep 2; \
	done; exit $$s

push-main: assert-main
	@echo "⬆️  Pushing $(BRANCH_MAIN)..."
	@for i in 1 2 3; do \
		git push origin $(BRANCH_MAIN) && s=0 && break || s=$$?; \
		echo "⚠️  Push $(BRANCH_MAIN) failed (try $$i/3). Retrying in 2s..."; \
		sleep 2; \
	done; exit $$s

merge: assert-main
	@echo "🔀 Merging $(BRANCH_DEV) -> $(BRANCH_MAIN)..."
	@git merge --no-ff --no-edit $(BRANCH_DEV)
	@$(MAKE) push-main

# Vérifie l'existence du tag local ET distant avant de créer/pousser
tag:
	@if git rev-parse --verify --quiet $(TAG) >/dev/null; then \
		echo "ℹ️  Local tag $(TAG) already exists."; \
	else \
		echo "🏷️  Creating annotated tag $(TAG)..."; \
		git tag -a $(TAG) -m "Release version $(TAG)"; \
	fi; \
	if git ls-remote --tags origin $(TAG) | grep -q "$(TAG)"; then \
		echo "✅ Remote already has tag $(TAG). Nothing to push."; \
	else \
		echo "⬆️  Pushing tag $(TAG) to origin..."; \
		for i in 1 2 3; do \
			git push origin $(TAG) && s=0 && break || s=$$?; \
			echo "⚠️  Push tag failed (try $$i/3). Retrying in 2s..."; \
			sleep 2; \
		done; exit $$s; \
	fi

resume-tag:
	@$(MAKE) tag VERSION=$(VERSION)

release:
	@$(MAKE) changelog
	@$(MAKE) commit VERSION=$(VERSION)
	@$(MAKE) push
	@$(MAKE) merge
	@$(MAKE) tag VERSION=$(VERSION)
	@echo "✅ Release $(TAG) complete."

# Variante sans interaction changelog (reprenable après coupure réseau)
release-fast:
	@$(MAKE) commit VERSION=$(VERSION)
	@$(MAKE) push
	@$(MAKE) merge
	@$(MAKE) tag VERSION=$(VERSION)
	@echo "✅ Release $(TAG) complete (fast)."

gh-release:
	@if ! command -v gh >/dev/null 2>&1; then \
		echo "❌ 'gh' CLI not found. Install GitHub CLI to use this target."; \
		exit 1; \
	fi
	@echo "📦 Creating GitHub Release for $(TAG)..."
	@gh release create $(TAG) --title "$(TAG)" --notes-file CHANGELOG.md || \
		{ echo "⚠️  'gh release' failed. Maybe it already exists?"; exit 0; }

test:
	@cd build && ctest --output-on-failure

changelog:
	@bash scripts/update_changelog.sh

submodules:
	@./scripts/submodules-sync.sh

submodules-remote:
	@./scripts/submodules-sync.sh remote
# ----------------------------------------------------------
