# flow5 (Axis fork)

Potential-flow solver (LLT, VLM, 3D panels, XFoil) with a Qt GUI, for wings, planes,
hydrofoils and sails. This repo is the Axis fork `github.com/mstrakl/flow5` (working branch
`master`) of upstream `techwinder/flow5` (remote `upstream`). The fork adds packaging/CI,
stability and control (T6/T7) features, and export options.

- Language: C++20, Qt 6, qmake (`flow5.pro`). Deps: OpenCascade, gmsh, OpenBLAS.
- `XFoil-lib/`: XFoil port. `flow5-lib/`: solver, geometry and objects (no Qt, `CONFIG -= qt`;
  public headers in `api/`). `flow5-io-lib/`: file I/O and serialization. `flow5-app/`: GUI.
- Build/packaging: `docker/` (dev container), `packaging/` (per-OS scripts), `.github/workflows/build.yml`.
- User docs for fork features: `meta/doc/` (e.g. `T7_stability_and_control.md`).
- Test models: `projects/*.fl5` (owner's files, untracked).

## Commands

    ./docker/dev.sh build            # incremental release build -> build/release/
    ./docker/dev.sh build debug      # debug tree -> build/debug/
    ./docker/dev.sh test             # build, then run the QtTest suites in tests/ headless
    ./docker/dev.sh run [debug]      # start the GUI from the container
    ./docker/dev.sh appimage         # Linux package -> build/dist/
    ./docker/dev.sh shell
    packaging/version.sh             # current version, from flow5-lib/api/fl5core.h

CI builds only on a push to `release` (publishes a GitHub Release) or a manual "Run workflow".
The version (`MAJOR_VERSION`/`MINOR_VERSION` in `fl5core.h`) must increase for each release.

## Priorities

When rules conflict, in this order: 1. correct results (a wrong number is worse than a crash),
2. never lose or alter user data (`.fl5` projects, settings), 3. readability and
maintainability, 4. reproducibility, 5. performance (only measured bottlenecks), 6. brevity.

## Working with the owner

- Before a new chunk of work, give a short plan (steps, files, size/time of builds or
  downloads) and wait for OK. Small steps inside an agreed plan need no confirmation.
- Work in milestones; after each: what changed, how it was verified, open questions, deviations.
- Ask before changing scope, architecture, the `.fl5` file format, the public API in
  `flow5-lib/api/`, or default results of existing analyses.
- Report honestly: build failures, unverified behaviour and assumptions are stated, not smoothed over.
- Never guess aero or flight-dynamics conventions (axes, signs, units, reference
  lengths/areas, derivative definitions). Check the code, AVL/flow5 docs or ask; otherwise
  mark `// ASSUMPTION: ...` in code and in the report.
- Recommend one design when several are reasonable; give the main trade-off, not a survey.
- Linux AppImage, Windows installer and macOS arm64 `.dmg` are fixed requirements.

## Git

- The owner commits, pushes, tags and switches branches. Leave changes in the working tree,
  list the files touched and suggest a commit message.
- Commit messages: one line, ≤ 20 words, prefix `(+)` add, `(~)` rework/change,
  `(-)` remove or bug fix. No agent watermarks or `Co-Authored-By` lines.
- One logical change per commit. No formatting churn: don't reformat or re-indent untouched code.
- **Keep each file's line endings** (many upstream files are CRLF). Never convert a whole
  file to LF/CRLF; check with `git diff --stat upstream/main` that only intended lines change.
- Don't commit build trees (`build/`), `projects/`, results or credentials.

## Fork and upstream

- Keep the diff to upstream small and easy to merge: change only what the task needs,
  match upstream style, and prefer adding new files/functions over rewriting upstream ones.
- New physics or corrections that change results get an option (checkbox/setting), with
  upstream behaviour unchanged when it is off, and a clear note in `meta/doc/`.
- Don't modify third-party code (OpenCascade, gmsh, Qt). Workarounds go in one clearly
  named place with a comment saying why.
- `flow5-lib` stays free of Qt and GUI code; computation goes in `flow5-lib`, dialogs and
  plots in `flow5-app`.

## Code style (C++ / Qt)

Follow the surrounding code; for new code use the upstream conventions:

- `PascalCase` classes, `camelCase` functions, members `m_`, statics `s_`; upstream type
  prefixes are fine (`m_b` bool, `m_p` pointer, `m_pfe` FloatEdit, ...).
- 4-space indent, braces on their own line, `#pragma once`, GPL header on new files
  (copy it from an existing file). Doxygen `/** */` and `/**< */` for public items.
- Clear, explicit code over clever code. Functions short and single-purpose.
- No magic numbers: named `constexpr` constants with unit and source in a comment.
  Use `PI` and the other constants from `flow5-lib/api/constants.h`.
- Comment *why*, not *what*. Equations: reference (document, section, eq.) and conventions
  (axis system, sign).
- Units: follow the convention of the code you touch (flow5 often uses degrees for angles
  and SI otherwise). State the unit in the name (`alpha_deg`, `time_s`) or in the comment at
  every declaration and I/O boundary. Convert explicitly, in one place.
- Ownership: follow the existing pattern of the class you're editing; for new standalone
  code prefer RAII and value types/`std::vector`. Every `new` has a clear owner.
- Use `const` and `override`; avoid global mutable state beyond the existing `s_` statics.
- User-visible strings in `tr()`.

## Errors and numerics

- Never fail silently. GUI: if an action can't run, tell the user what is wrong and why
  (message box, status or log). Library: return an error/flag with a message saying what
  was expected, what was found and where.
- Don't swallow errors or clip, interpolate or drop data silently; log what was done.
- Check for NaN/inf before results reach polars, graphs or files.
- Compare floats with a named tolerance (`PRECISION` or a justified constant), never `==`.
- Results must be deterministic: no dependence on hash/map iteration order or timing.

## Graphs

Axes labelled with quantity and unit (never a placeholder like "y"). Legend where there is
more than one series. Defaults should be useful without the user changing settings.

## Verifying changes

`tests/` has QtTest suites: `lib/` (flow5-lib and flow5-io-lib, headless) and `gui/` (offscreen,
links the app's object files). The models are built in code by `tests/common/fixture.cpp`.
Add a test with each GUI or store fix where it is practical. Before reporting "done":

- [ ] `./docker/dev.sh build` succeeds with no new warnings in touched files.
- [ ] `./docker/dev.sh test` passes.
- [ ] Behaviour checked in the GUI (`./docker/dev.sh run`) on a small model from `projects/`,
      or say clearly that it was not run and what the owner should check.
- [ ] Results physically plausible; with an option off, results match the previous version.
- [ ] Only intended lines changed vs `upstream/main` (no line-ending or formatting churn).
- [ ] `meta/doc/` and `README.md` updated when user-visible behaviour, options or build change.
- [ ] Assumptions, limitations and anything unverified listed in the report.
