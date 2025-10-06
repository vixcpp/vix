#!/bin/bash

echo "Version ? (ex: 0.1.1)"
read VERSION
TODAY=$(date +%F)

echo -e "## [$VERSION] - $TODAY\n### Added\n- \n\n### Changed\n- \n\n### Removed\n- \n\n" | cat - CHANGELOG.md > temp && mv temp CHANGELOG.md
