#!/usr/bin/env bash
#
# MaxCall auto-release
# --------------------
# Commits pending changes, pushes main, bumps the version tag (vX.Y.Z -> vX.Y.Z+1)
# and pushes the tag. Pushing the tag triggers the GH Action which builds
# MaxCall.exe and publishes the versioned GitHub Release automatically.
#
# Usage:
#   ./release.sh "commit message"            # full flow + watch the build
#   ./release.sh "commit message" --no-watch # without watching the build
#
set -euo pipefail
cd "$(dirname "$0")"

if [ $# -lt 1 ]; then
  echo "Usage: $0 \"commit message\" [--no-watch]"
  exit 1
fi
MSG="$1"
WATCH=true
[ "${2:-}" = "--no-watch" ] && WATCH=false

command -v gh >/dev/null || { echo "ERROR: gh CLI is required"; exit 1; }

# 1. Commit any pending changes
git add -A
if git diff --cached --quiet; then
  echo "No changes to commit."
else
  git commit -m "$MSG"
fi

# 2. Push main (triggers build -> updates the 'latest' release)
git push origin main

# 3. Compute next patch version from the newest v* tag
git fetch origin --tags --quiet
LAST="$(git tag --list 'v*' --sort=-v:refname | head -n1)"
[ -z "$LAST" ] && LAST="v0.0.0"
VER="${LAST#v}"
IFS=. read -r MA MI PA <<< "$VER"
NEXT="v$MA.$MI.$((PA + 1))"
echo "Last version: $LAST -> next: $NEXT"

# 4. Tag + push tag (triggers build -> versioned release)
git tag -a "$NEXT" -m "MaxCall $NEXT"
git push origin "$NEXT"

REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
echo ""
echo "Released: https://github.com/$REPO/releases/tag/$NEXT"

# 5. Watch the build until it finishes
if $WATCH; then
  echo "Waiting for the build to start..."
  sleep 15
  RUN_ID="$(gh run list --limit 5 --json databaseId,createdAt -q 'sort_by(.createdAt)[-1].databaseId')"
  gh run watch "$RUN_ID" --exit-status
  echo ""
  echo "Done: https://github.com/$REPO/releases/tag/$NEXT"
fi
