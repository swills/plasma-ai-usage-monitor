# v20 qualification contract

Ordinary pull requests run native tests, QML lint/import/smoke, static checks,
package payload checks and sanitizers. Saved media still undergoes image hash,
geometry, scenario and process/window/accessibility evidence validation through
`python3 scripts/check_release_media.py --mode development`. Development checks
are not release qualification.

The default `check_release_media.py` mode is strict candidate validation. It
requires the current release version, fixtures and source inventory to match the
capture manifest. CTest's `release-candidate` tests remain mandatory for a final
candidate; ordinary development CI excludes only that label.

The reusable `candidate.yml` workflow captures real Plasma windows in a private
D-Bus session and virtual compositor with isolated HOME and XDG directories.
It builds the native plugin before installation, and refuses to certify captures
if the source inventory changed during building/capture. It never reuses the
active desktop's panel, accounts or settings. Process identity and accessibility
markers are checked before and after screenshots. Capture diagnostics survive
failure in `build/capture-diagnostics/`; screenshot artifacts are uploaded only
after successful capture and strict validation.

The package job downloads `plasma-media-<candidate SHA>` from the same workflow
run and repeats source/archive checks. The tag publication workflow requires
candidate qualification, a clean exact annotated tag, and strict validation of
the committed screenshot evidence before publication. A source change after
capture requires another capture; do not edit hashes to reuse older images.

Local isolated capture:

```bash
QT_QUICK_BACKEND=software LIBGL_ALWAYS_SOFTWARE=1 \
  bash scripts/demo/capture_v18_media.sh "$PWD/build/debug" "$PWD/assets/screenshots"
python3 scripts/check_release_media.py
python3 scripts/test_protobuf_discovery.py
```

The Protobuf smoke fixture generates, links and runs a serialization round trip
through both config and module discovery. When a vendor config package is absent
(as on Fedora's module-only packaging), it explicitly reports use of a controlled
config compatibility fixture backed by the real installed compiler and library.
This is not evidence for a native vendor config package or FreeBSD support.
Real FreeBSD qualification and physical horizontal/vertical panel, input,
accessibility, login and installation checks remain separately recorded gates.

Superseded development runs are cancelled. Build caches are deliberately deferred
until run timings establish a useful cache target; no credentials or user
history databases belong in caches.

## Real panel matrix

`capture_panel_matrix.py` uses private virtual KWin sessions, the actual native
plugin and actual Plasma panel containers. The full matrix covers horizontal and
vertical panels at 24/32/48/64 logical pixels, scales 100/125/150/200 percent,
Breeze light/dark, quota 28/0/100 percent, no connected source, and native-plugin
recovery. Each case verifies the requested panel geometry and PID-bound AT-SPI
marker before/after capture. Explicit percentage markers prevent accidentally
capturing another fixture. The `panel-matrix.yml` workflow partitions by scale
and theme and is a required part of candidate qualification.

```bash
python3 scripts/demo/capture_panel_matrix.py \
  --build-dir build/debug --output work/panel-matrix-final
python3 scripts/check_panel_matrix.py work/panel-matrix-final
```

Use a fresh output directory. The final manifest is written only if every selected
case passes and the source inventory remains unchanged throughout the run. It
hashes individual evidence records, which in turn hash their screenshots. Logs
and partial captures remain available on failure but are not certified. This
matrix verifies rendering and accessibility names; keyboard navigation, visual
inspection and physical desktop behavior are explicitly `unverified` until
separately exercised and recorded. Do not describe AT-SPI name checks as a full
screen-reader audit.
