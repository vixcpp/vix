#!/bin/bash

CHANGELOG_FILE="CHANGELOG.md"

echo "📝 Entrez la nouvelle version (ex: 0.1.1) :"
read VERSION

if grep -q "## \[$VERSION\]" "$CHANGELOG_FILE"; then
    echo "❌ La version $VERSION existe déjà dans le changelog."
    exit 1
fi

TODAY=$(date +%F)

if [ ! -f "$CHANGELOG_FILE" ]; then
    echo "❌ Le fichier $CHANGELOG_FILE n'existe pas."
    exit 1
fi

NEW_ENTRY="## [$VERSION] - $TODAY

### Added
- 

### Changed
- 

### Removed
- 
"

awk -v new_entry="$NEW_ENTRY" '
BEGIN { inserted=0 }
/## \[Unreleased\]/ && !inserted {
    print $0 "\n" new_entry
    inserted=1
    next
}
{ print }
' "$CHANGELOG_FILE" > "$CHANGELOG_FILE.tmp" && mv "$CHANGELOG_FILE.tmp" "$CHANGELOG_FILE"

echo "✅ Bloc version $VERSION ajouté dans $CHANGELOG_FILE."
