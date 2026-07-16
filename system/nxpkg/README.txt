nxpkg - a package manager for NuttX application images
========================================================

  nxpkg installs, updates, uninstalls, and rolls back standalone loadable
  ELF applications (MODULE=m in an app's Makefile - see games/NXDoom or
  examples/calculator for real ports) from a package repository served
  over HTTP, onto local storage (an SD card on this board's config).
  system/nxstore is an LVGL touchscreen front-end for nxpkg; this
  document covers the repository/server side, which nxstore and the
  nxpkg CLI both consume identically.


Repository layout
==================

  A repository is nothing more than a directory of static files:

    <repo-root>/
      index.json                                   - the package catalog
      artifacts/<arch>/<chip>/<compat>/<name>/<version>/<artifact-file>
      icons/<arch>/<chip>/<compat>/<name>.bin        - optional, see below

  index.json is a single JSON object with one "packages" array. Each
  entry is one installable version of one package:

    {
      "packages": [
        {
          "name": "calc",
          "version": "6",
          "arch": "xtensa",
          "compat": "esp32s3-touch-lcd-7",
          "artifact": "artifacts/xtensa/esp32s3/esp32s3-touch-lcd-7/calc/6/calc",
          "sha256": "<64 hex chars>",
          "type": "elf",
          "description": "Simple 4-function calculator",
          "category": "Utilities",
          "icon": "icons/xtensa/esp32s3/esp32s3-touch-lcd-7/calc.bin"
        }
      ]
    }

  "artifact" and "icon" (both optional except "artifact") are resolved
  relative to the repository's own base URL (the URL index.json itself
  was fetched from, with the filename stripped) unless they're already
  a full http(s):// URL - see pkg_resolve_artifact_source()/
  pkg_resolve_icon_source() in pkg_repo.c. That means the whole
  directory tree above can be moved, mirrored, or served from a
  completely different host without editing a single path in
  index.json, as long as the *relative* structure between index.json
  and artifacts/icons/ stays the same.

  "arch"/"compat" must match this board's CONFIG_ARCH ("xtensa") and
  CONFIG_ARCH_BOARD ("esp32s3-touch-lcd-7") exactly - see
  pkg_compat_check() in pkg_compat.c - so one repository can host
  packages for several different boards side by side; each board only
  ever installs the entries that match its own identity.


What actually serves this - any static file host works
========================================================

  pkg_repo_fetch_url() (pkg_repo.c) does a plain HTTP GET with no auth,
  no custom headers, and no server-side logic required - it just needs
  a 2xx response. This means literally any static file server works:
  nginx or Apache with a DocumentRoot pointed at the repo directory, a
  one-line Python http.server, GitHub Pages, S3/R2/Cloudflare Pages, or
  a NAS's built-in web server. There is nothing nxpkg-specific to
  install on the server side.

  For local development/testing (what this project actually used while
  building and testing every package in this series on real hardware):

    cd /path/to/repo-root
    python3 -m http.server 8000

  That's the entire "server setup." Confirm it's reachable from your
  own machine first:

    curl -sI http://<your-machine-ip>:8000/index.json

  sha256 verification (pkg_hash_file_sha256(), checked against the
  manifest's "sha256" field before an artifact is ever activated)
  already protects payload integrity in transit even over plain HTTP.
  Only confidentiality/availability would need HTTPS on top of this if
  that matters for a given deployment - nothing in the protocol assumes
  HTTP specifically, an https:// URL works exactly the same way.


Populating a repository: tools/export_pkg_repo.py
==================================================

  Building the JSON by hand is error-prone (sha256 digests, exact
  relative paths); tools/export_pkg_repo.py does it for you from a
  built binary:

    python3 tools/export_pkg_repo.py \
      --arch xtensa --chip esp32s3 --compat esp32s3-touch-lcd-7 \
      --package "calc:6:elf:/path/to/apps/bin/calc:Simple 4-function calculator:Utilities" \
      /path/to/repo-root

  --package can be repeated to export several packages/versions in one
  call. The colon-separated spec is:

    <name>:<version>:<elf|shared-lib>:<source>[:<description>:<category>[:<icon>]]

  <source> is the built artifact on your machine (e.g. apps/bin/calc
  after a normal `make`). <icon> is optional: any image Pillow can open
  (PNG, JPEG, ...) - the tool resizes it to a fixed 48x48 and encodes it
  into the small raw RGB565 format system/nxstore's icon rendering
  expects (see nxstore_load_icon() in nxstore_main.c for the exact
  on-wire layout). Re-running the script is idempotent: it loads
  whatever index.json already exists in the target directory, merges in
  the packages you just passed (matched on name+version+arch+compat+
  type), and rewrites the file - existing unrelated entries are left
  alone.

  This board's FAT-formatted SD card only supports short (8.3) file/
  directory names with no long-name extension - any path component
  (package name, artifact leaf filename) over 8 characters before its
  extension fails to install with errno 22, not a clear error message.
  Keep names short (this is why this series' packages are named "calc",
  "brick", "cgol" rather than "calculator", "brickmatch", "conway") -
  this is a board/filesystem constraint, not an export_pkg_repo.py or
  nxpkg one, and would not apply to a board with a different storage
  filesystem.


Pointing the board at your repository
======================================

  Two ways to get an index synced onto the board's local storage,
  useful in different situations:

  1. One-shot from NSH, useful for iterating on a package during
     development (this is what building and testing every package in
     this series actually looked like):

       nxpkg sync http://<your-machine-ip>:8000/index.json
       nxpkg install <name>
       nxpkg update <name>     # re-check for/install a newer version
       nxpkg list               # show what's installed, current vs previous
       nxpkg rollback <name>    # swap back to the previous version
       nxpkg remove <name>

  2. Automatically at boot, via system/nxstore: the repo URL nxstore
     syncs from at startup is currently a single hardcoded string in
     board_nxstore_autostart() (boards/xtensa/esp32s3/
     esp32s3-touch-lcd-7/src/esp32s3_bringup.c) - change that one line
     to point at a different host, or a public one, and every boot
     re-syncs the catalog automatically (falling back to whatever was
     last synced to local storage if the network/server is unreachable
     at boot - nxstore never blocks the app list on a failed sync).

  Development-network note: if your development machine's IP address
  changes (switching Wi-Fi networks, a new DHCP lease, a hotspot),
  re-run `nxpkg sync` with the new address - the board and the machine
  serving the repository just need to be able to reach each other over
  IP; nothing else about the setup changes.


Concurrency note
=================

  nxpkg protects a single package's own install/update/rollback against
  running twice at once (a per-package lock file), and separately
  protects the shared installed-packages database against being
  corrupted by two *different* packages' installs racing each other
  (see pkg_install_acquire_installed_lock() in pkg_install.c) - so it is
  safe to have, for example, system/nxstore's own background install
  worker and a directly-invoked `nxpkg install` running at the same
  time for different packages. Two operations on the *same* package
  name at the same time will have the second one fail with -EBUSY
  (surfaced as "package has an install/update in progress") rather than
  either one silently clobbering the other's work.
