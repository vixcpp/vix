# Vix UI examples

This directory contains examples for the `vix::ui` module.

## Examples

```bash
vix run examples/ui/ui_html.cpp
vix run examples/ui/ui_assets.cpp
vix run examples/ui/ui_view.cpp
vix run examples/ui/ui_forms.cpp
vix run examples/ui/ui_live.cpp
vix run examples/ui/ui_pwa.cpp
vix run examples/ui/ui_dashboard.cpp
```

## Desktop

```bash
vix desktop run examples/ui/ui_dashboard.cpp \
  --name "Vix UI Dashboard" \
  --width 1100 \
  --height 760

vix desktop build examples/ui/ui_dashboard.cpp \
  --name "Vix UI Dashboard" \
  --app-id com.vixcpp.dashboard \
  --version 1.0.0 \
  --width 1100 \
  --height 760
```

## What each example shows

- `ui_html.cpp` — low-level HTML helpers
- `ui_assets.cpp` — asset manifest rendering
- `ui_view.cpp` — template-backed views
- `ui_forms.cpp` — form fields and CSRF helpers
- `ui_live.cpp` — flash messages, toasts and fragments
- `ui_pwa.cpp` — PWA metadata, manifest and safe-area CSS
- `ui_dashboard.cpp` — complete server-rendered dashboard
- `ui_shell.cpp` — desktop shell configuration
