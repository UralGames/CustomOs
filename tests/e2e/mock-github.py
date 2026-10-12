#!/usr/bin/env python3
"""A small stand-in for the GitHub Releases API, for the end-to-end tests.

    mock-github.py --root DIR [--port 8077] [--repo test/customos] [--host 10.0.2.2]

Every directory DIR/vX.Y.Z/ is a published release; its files are the release
assets and RELEASE_NOTES.md is the release text. Copy a directory in to
"publish" a release while the server runs. Like GitHub:
  GET /repos/OWNER/NAME/releases/latest   newest release without a pre-release part
  GET /repos/OWNER/NAME/releases          all releases, newest first
  GET /repos/OWNER/NAME                   the repository
  GET /OWNER/NAME/releases/download/TAG/FILE   302 to /storage/TAG/FILE
  GET /storage/TAG/FILE                   the file, with HTTP Range support
"""
import argparse
import hashlib
import json
import os
import re
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

SEMVER = re.compile(r"^v(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?$")


def version_key(tag):
    m = SEMVER.match(tag)
    major, minor, patch, pre = int(m[1]), int(m[2]), int(m[3]), m[4]
    # A release sorts after its pre-releases.
    return (major, minor, patch, pre is None, pre or "")


class Handler(BaseHTTPRequestHandler):
    server_version = "mock-github/1"

    def log_message(self, fmt, *args):
        print("mock-github: " + fmt % args, flush=True)

    def send_json(self, obj, status=200):
        body = json.dumps(obj).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def releases(self):
        root = self.server.root
        tags = [d for d in os.listdir(root) if SEMVER.match(d) and os.path.isdir(os.path.join(root, d))]
        return sorted(tags, key=version_key, reverse=True)

    def release_json(self, tag):
        cfg = self.server
        base = f"http://{cfg.public_host}:{cfg.server_port}"
        d = os.path.join(cfg.root, tag)
        assets = []
        for i, name in enumerate(sorted(os.listdir(d))):
            path = os.path.join(d, name)
            if name == "RELEASE_NOTES.md" or not os.path.isfile(path):
                continue
            with open(path, "rb") as f:
                digest = hashlib.sha256(f.read()).hexdigest()
            assets.append({
                "id": i + 1, "name": name, "size": os.path.getsize(path),
                "browser_download_url": f"{base}/{cfg.repo}/releases/download/{tag}/{name}",
                "url": f"{base}/repos/{cfg.repo}/releases/assets/{tag}/{name}",
                "digest": "sha256:" + digest,
            })
        notes = os.path.join(d, "RELEASE_NOTES.md")
        body = open(notes, encoding="utf-8").read() if os.path.exists(notes) else ""
        return {
            "tag_name": tag, "name": "CustomOS " + tag[1:], "body": body, "draft": False,
            "prerelease": SEMVER.match(tag)[4] is not None,
            "html_url": f"https://github.com/{cfg.repo}/releases/tag/{tag}",
            "published_at": "2026-10-12T09:00:00Z", "assets": assets,
        }

    def do_GET(self):
        cfg = self.server
        path = self.path.split("?", 1)[0]
        if path == f"/repos/{cfg.repo}":
            return self.send_json({"full_name": cfg.repo})
        if path == f"/repos/{cfg.repo}/releases":
            return self.send_json([self.release_json(t) for t in self.releases()])
        if path == f"/repos/{cfg.repo}/releases/latest":
            stable = [t for t in self.releases() if SEMVER.match(t)[4] is None]
            if not stable:
                return self.send_json({"message": "Not Found"}, 404)
            return self.send_json(self.release_json(stable[0]))
        m = re.match(rf"^/(?:{re.escape(cfg.repo)}/releases/download|repos/{re.escape(cfg.repo)}/releases/assets)/([^/]+)/([^/]+)$", path)
        if m:
            self.send_response(302)
            self.send_header("Location", f"/storage/{m[1]}/{m[2]}")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        m = re.match(r"^/storage/([^/]+)/([^/]+)$", path)
        if m and SEMVER.match(m[1]) and "/" not in m[2] and m[2] != "..":
            return self.send_file(os.path.join(cfg.root, m[1], m[2]))
        self.send_json({"message": "Not Found"}, 404)

    def send_file(self, path):
        if not os.path.isfile(path):
            return self.send_json({"message": "Not Found"}, 404)
        size = os.path.getsize(path)
        start = 0
        rng = self.headers.get("Range", "")
        m = re.match(r"^bytes=(\d+)-$", rng)
        if m:
            start = int(m[1])
            if start >= size:
                self.send_response(416)
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            self.send_response(206)
            self.send_header("Content-Range", f"bytes {start}-{size - 1}/{size}")
        else:
            self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(size - start))
        self.end_headers()
        with open(path, "rb") as f:
            f.seek(start)
            while True:
                chunk = f.read(1 << 20)
                if not chunk:
                    break
                try:
                    self.wfile.write(chunk)
                except (BrokenPipeError, ConnectionResetError):
                    return


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--root", required=True)
    p.add_argument("--port", type=int, default=8077)
    p.add_argument("--repo", default="test/customos")
    p.add_argument("--host", default="10.0.2.2", help="host name the clients use to reach this server")
    a = p.parse_args()
    httpd = ThreadingHTTPServer(("0.0.0.0", a.port), Handler)
    httpd.root, httpd.repo, httpd.public_host = os.path.abspath(a.root), a.repo, a.host
    print(f"mock-github: serving {httpd.root} as github.com/{a.repo} on port {a.port}", flush=True)
    httpd.serve_forever()


if __name__ == "__main__":
    main()
