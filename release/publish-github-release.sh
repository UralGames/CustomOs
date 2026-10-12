#!/bin/bash
# publish-github-release.sh: publish the files from make-release.sh as a GitHub release.
#
#   release/publish-github-release.sh RELEASE_DIR [--repo OWNER/NAME] [--target COMMIT] [--draft]
#
# Uses the GitHub CLI (gh) when it is installed and authenticated, otherwise the
# REST API with $GITHUB_TOKEN (a token with "contents: write" on the repository;
# GitHub Actions provides one). The release is created as a draft, the assets
# are uploaded, and only then is it published, so update clients never see a
# release with missing files. --draft leaves it as a draft for review.
# Versions with a pre-release part (0.3.0-rc.1) are published as pre-releases,
# which the stable channel ignores.
set -euo pipefail

die() { echo "publish-github-release: $*" >&2; exit 1; }

DIR=""; REPO_NAME="${GITHUB_REPOSITORY:-}"; TARGET="${GITHUB_SHA:-}"; KEEP_DRAFT=0
while [ $# -gt 0 ]; do
    case "$1" in
        --repo) REPO_NAME="$2"; shift 2 ;;
        --target) TARGET="$2"; shift 2 ;;
        --draft) KEEP_DRAFT=1; shift ;;
        -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
        -*) die "unknown option $1" ;;
        *) DIR="$1"; shift ;;
    esac
done
[ -n "$DIR" ] && [ -d "$DIR" ] || die "usage: publish-github-release.sh RELEASE_DIR"
DIR="$(cd "$DIR" && pwd)"

if [ -z "$REPO_NAME" ]; then
    url=$(git -C "$(dirname "$0")" remote get-url origin 2>/dev/null || true)
    REPO_NAME=$(echo "$url" | sed -E 's#^(https?://[^/]+/|git@[^:]+:)##; s#\.git$##')
fi
echo "$REPO_NAME" | grep -Eq '^[A-Za-z0-9._-]+/[A-Za-z0-9._-]+$' || die "cannot tell the repository; use --repo OWNER/NAME"

MANIFEST=$(ls "$DIR"/CustomOS-v*-manifest.json 2>/dev/null | head -n1)
[ -n "$MANIFEST" ] || die "no CustomOS-v*-manifest.json in $DIR"
VERSION=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["version"])' "$MANIFEST")
TAG="v$VERSION"
ASSETS=("$DIR/CustomOS-$TAG.raucb" "$MANIFEST" "$DIR/CustomOS-$TAG-x86_64.iso" "$DIR/SHA256SUMS")
for a in "${ASSETS[@]}"; do [ -s "$a" ] || die "missing $a"; done
[ -s "$DIR/RELEASE_NOTES.md" ] || die "missing $DIR/RELEASE_NOTES.md"
( cd "$DIR" && sha256sum --quiet -c SHA256SUMS ) || die "SHA256SUMS does not match the files"
PRERELEASE=false
case "$VERSION" in *-*) PRERELEASE=true ;; esac

echo ">>> Publishing CustomOS $VERSION as $TAG on github.com/$REPO_NAME"

if command -v gh >/dev/null 2>&1 && gh auth status >/dev/null 2>&1; then
    if gh release view "$TAG" --repo "$REPO_NAME" >/dev/null 2>&1; then
        die "release $TAG already exists on $REPO_NAME"
    fi
    args=(--repo "$REPO_NAME" --title "CustomOS $VERSION" --notes-file "$DIR/RELEASE_NOTES.md" --draft)
    [ -n "$TARGET" ] && args+=(--target "$TARGET")
    [ "$PRERELEASE" = true ] && args+=(--prerelease)
    gh release create "$TAG" "${args[@]}" "${ASSETS[@]}"
    if [ "$KEEP_DRAFT" -eq 0 ]; then
        latest=true; [ "$PRERELEASE" = true ] && latest=false
        gh release edit "$TAG" --repo "$REPO_NAME" --draft=false --latest="$latest"
    fi
    gh release view "$TAG" --repo "$REPO_NAME" --json url --jq .url
    exit 0
fi

[ -n "${GITHUB_TOKEN:-}" ] || die "install and log in to the GitHub CLI (gh auth login) or set GITHUB_TOKEN"
API="${GITHUB_API_URL:-https://api.github.com}"
auth=(-H "Authorization: Bearer $GITHUB_TOKEN" -H "Accept: application/vnd.github+json" -H "X-GitHub-Api-Version: 2022-11-28")
code=$(curl -sS -o /dev/null -w '%{http_code}' "${auth[@]}" "$API/repos/$REPO_NAME/releases/tags/$TAG")
[ "$code" = 404 ] || die "release $TAG already exists on $REPO_NAME (HTTP $code)"

body=$(TAG="$TAG" TARGET="$TARGET" VERSION="$VERSION" PRE="$PRERELEASE" NOTES="$DIR/RELEASE_NOTES.md" python3 -c '
import json, os
e = os.environ
r = {"tag_name": e["TAG"], "name": "CustomOS " + e["VERSION"], "body": open(e["NOTES"], encoding="utf-8").read(),
     "draft": True, "prerelease": e["PRE"] == "true"}
if e["TARGET"]:
    r["target_commitish"] = e["TARGET"]
print(json.dumps(r))')
created=$(curl -sS --fail-with-body "${auth[@]}" -X POST -d "$body" "$API/repos/$REPO_NAME/releases")
id=$(echo "$created" | python3 -c 'import json,sys; print(json.load(sys.stdin)["id"])')
upload=$(echo "$created" | python3 -c 'import json,sys; print(json.load(sys.stdin)["upload_url"].split("{")[0])')
for a in "${ASSETS[@]}"; do
    echo "    uploading $(basename "$a") ($(du -h "$a" | cut -f1))"
    curl -sS --fail-with-body "${auth[@]}" -H "Content-Type: application/octet-stream" \
        --data-binary @"$a" "$upload?name=$(basename "$a")" >/dev/null
done
if [ "$KEEP_DRAFT" -eq 0 ]; then
    latest=true; [ "$PRERELEASE" = true ] && latest=false
    curl -sS --fail-with-body "${auth[@]}" -X PATCH -d "{\"draft\": false, \"make_latest\": \"$latest\"}" \
        "$API/repos/$REPO_NAME/releases/$id" >/dev/null
fi
echo "https://github.com/$REPO_NAME/releases/tag/$TAG"
