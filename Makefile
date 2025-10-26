# =========================
# Vix umbrella — Safe Release Makefile
# =========================
VERSION       ?= v0.1.0
REMOTE        = origin
BRANCH_DEV    = dev
BRANCH_MAIN   = main

.PHONY: help status guard-clean sync ffmerge merge-or-pr changelog commit push tag release submodules submodules-remote

help:
	@echo "Available commands:"
	@echo "  make status                 - Show divergence dev/main (remote)"
	@echo "  make submodules             - Sync submodules to committed SHAs"
	@echo "  make submodules-remote      - Sync submodules to remote default"
	@echo "  make changelog              - Update CHANGELOG.md using script"
	@echo "  make commit                 - Commit local changes on $(BRANCH_DEV)"
	@echo "  make push                   - Push $(BRANCH_DEV)"
	@echo "  make sync                   - Rebase $(BRANCH_DEV) on $(REMOTE)/$(BRANCH_MAIN) & align local main"
	@echo "  make ffmerge                - Fast-forward merge $(BRANCH_DEV) -> $(BRANCH_MAIN) & push"
	@echo "  make merge-or-pr            - FF merge else open PR (requires gh)"
	@echo "  make tag VERSION=vX.Y.Z     - Create and push annotated tag"
	@echo "  make release VERSION=vX.Y.Z - changelog + commit + push + submodules + sync + ffmerge + tag"

status:
	@git fetch $(REMOTE) --tags
	@echo "== Commits on $(BRANCH_DEV) not in $(BRANCH_MAIN):"
	@git log --oneline $(REMOTE)/$(BRANCH_MAIN)..$(REMOTE)/$(BRANCH_DEV) || true
	@echo
	@echo "== Commits on $(BRANCH_MAIN) not in $(BRANCH_DEV):"
	@git log --oneline $(REMOTE)/$(BRANCH_DEV)..$(REMOTE)/$(BRANCH_MAIN) || true

guard-clean:
	@if [ -n "$$(git status --porcelain)" ]; then \
		echo "❌ Working tree not clean. Commit or stash first."; \
		git status --porcelain; \
		exit 1; \
	fi

submodules:
	@./scripts/submodules-sync.sh

submodules-remote:
	@./scripts/submodules-sync.sh remote

changelog:
	@bash scripts/update_changelog.sh

commit:
	@git switch $(BRANCH_DEV)
	@if [ -n "$$(git status --porcelain)" ]; then \
		echo "📝 Committing changes…"; \
		git add .; \
		git commit -m "chore(release): prepare $(VERSION)"; \
	else \
		echo "✅ Nothing to commit."; \
	fi

push:
	@git push $(REMOTE) $(BRANCH_DEV)

sync: ## rebase dev on origin/main; align local main
	@git fetch $(REMOTE) --tags
	@git switch $(BRANCH_DEV)
	@git rebase $(REMOTE)/$(BRANCH_MAIN) || { echo "❌ Rebase failed. Resolve and 'git rebase --continue'."; exit 1; }
	@git push --force-with-lease $(REMOTE) $(BRANCH_DEV)
	@git switch $(BRANCH_MAIN)
	@git reset --hard $(REMOTE)/$(BRANCH_MAIN)

ffmerge:
	@git switch $(BRANCH_MAIN)
	@git merge --ff-only $(BRANCH_DEV)
	@git push $(REMOTE) $(BRANCH_MAIN)

merge-or-pr:
	@git switch $(BRANCH_MAIN)
	@git merge --ff-only $(BRANCH_DEV) || ( \
		echo "ℹ️  FF impossible, creating PR…"; \
		command -v gh >/dev/null 2>&1 || { echo "❌ 'gh' not found. Install GitHub CLI or merge manually."; exit 1; }; \
		gh pr create -B $(BRANCH_MAIN) -H $(BRANCH_DEV) -t "Merge $(BRANCH_DEV) into $(BRANCH_MAIN)" -b "Auto-PR from Makefile"; \
	)

tag:
	@if git rev-parse $(VERSION) >/dev/null 2>&1; then \
		echo "❌ Tag $(VERSION) already exists."; \
		exit 1; \
	else \
		echo "🏷️  Creating annotated tag $(VERSION)…"; \
		git tag -a $(VERSION) -m "Release $(VERSION)"; \
		git push $(REMOTE) $(VERSION); \
	fi

release: changelog commit push submodules sync ffmerge tag
	@echo "✅ Release done: $(VERSION)"
