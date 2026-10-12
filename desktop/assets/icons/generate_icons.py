#!/usr/bin/env python3
"""Generate the Meridian icon set.

Every icon here is original artwork drawn for this project on a 24-unit grid
(symbolic icons) or a 48-unit grid (application icons). Symbolic icons use
`currentColor`; the desktop's icon provider substitutes the theme colour at
render time, so one SVG serves light and dark themes.

Run:  python3 generate_icons.py   (writes ./symbolic/*.svg and ./apps/*.svg)
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))

SYM_HEAD = ('<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" '
            'fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" '
            'stroke-linejoin="round">')

# name -> inner SVG markup (24x24 grid, stroked unless noted)
SYMBOLIC = {
    # --- system -----------------------------------------------------------
    "search": '<circle cx="10.5" cy="10.5" r="6"/><path d="M15 15l5 5"/>',
    "power": '<path d="M12 3.5v7.5"/><path d="M7.2 6.6A7.2 7.2 0 1 0 16.8 6.6"/>',
    "restart": '<path d="M19.5 12a7.5 7.5 0 1 1-2.6-5.7"/><path d="M19.5 4v4.5H15"/>',
    "logout": '<path d="M10 4.5H6a1.5 1.5 0 0 0-1.5 1.5v12A1.5 1.5 0 0 0 6 19.5h4"/>'
              '<path d="M14.5 8l4 4-4 4"/><path d="M18.5 12H9.5"/>',
    "lock": '<rect x="5" y="10.5" width="14" height="9.5" rx="2"/><path d="M8.5 10.5V8a3.5 3.5 0 0 1 7 0v2.5"/>',
    "settings": '<circle cx="12" cy="12" r="3"/>'
                '<path d="M12 2.8l1.6 2.4 2.8-.6.6 2.8 2.4 1.6-1.4 2.5 1.4 2.5-2.4 1.6-.6 2.8-2.8-.6L12 21.2l-1.6-2.4-2.8.6-.6-2.8-2.4-1.6L6 12.5 4.6 10l2.4-1.6.6-2.8 2.8.6z"/>',
    "sliders": '<path d="M4 7h9M17 7h3M4 17h3M11 17h9"/><circle cx="15" cy="7" r="2"/><circle cx="9" cy="17" r="2"/>',
    "bell": '<path d="M6 16.5V11a6 6 0 0 1 12 0v5.5l1.5 1.5h-15z"/><path d="M10 20.5a2.2 2.2 0 0 0 4 0"/>',
    "bell-off": '<path d="M8 6.2A6 6 0 0 1 18 11v4"/><path d="M6 11v5.5L4.5 18H17"/><path d="M10 20.5a2.2 2.2 0 0 0 4 0"/><path d="M4 4l16 16"/>',
    "moon": '<path d="M19.5 14.5A8 8 0 0 1 9.5 4.5a8 8 0 1 0 10 10z"/>',
    "sun": '<circle cx="12" cy="12" r="4"/><path d="M12 2.5v2M12 19.5v2M2.5 12h2M19.5 12h2M5.3 5.3l1.4 1.4M17.3 17.3l1.4 1.4M5.3 18.7l1.4-1.4M17.3 6.7l1.4-1.4"/>',
    "calendar": '<rect x="4" y="5.5" width="16" height="14.5" rx="2"/><path d="M4 10h16M8.5 3.5v4M15.5 3.5v4"/>',
    "clock": '<circle cx="12" cy="12" r="8.5"/><path d="M12 7.5V12l3 2"/>',
    "camera": '<path d="M4.5 8.5A1.5 1.5 0 0 1 6 7h2l1.5-2h5L16 7h2a1.5 1.5 0 0 1 1.5 1.5v9A1.5 1.5 0 0 1 18 19H6a1.5 1.5 0 0 1-1.5-1.5z"/><circle cx="12" cy="12.5" r="3.2"/>',
    "info": '<circle cx="12" cy="12" r="8.5"/><path d="M12 11v5.5"/><path d="M12 7.6v.2" stroke-width="2.2"/>',
    "warning": '<path d="M12 4l9 15.5H3z"/><path d="M12 10v4.5"/><path d="M12 17.2v.2" stroke-width="2.2"/>',
    "check": '<path d="M5 12.5l4.5 4.5L19 7.5"/>',
    "close": '<path d="M6.5 6.5l11 11M17.5 6.5l-11 11"/>',
    "minimize": '<path d="M6 12h12"/>',
    "maximize": '<rect x="6" y="6" width="12" height="12" rx="1.5"/>',
    "restore": '<rect x="5.5" y="8.5" width="10" height="10" rx="1.5"/><path d="M8.5 5.5h8.5a1.5 1.5 0 0 1 1.5 1.5v8.5"/>',
    "plus": '<path d="M12 5v14M5 12h14"/>',
    "minus": '<path d="M5 12h14"/>',
    "chevron-left": '<path d="M14.5 6l-6 6 6 6"/>',
    "chevron-right": '<path d="M9.5 6l6 6-6 6"/>',
    "chevron-up": '<path d="M6 14.5l6-6 6 6"/>',
    "chevron-down": '<path d="M6 9.5l6 6 6-6"/>',
    "arrow-left": '<path d="M19 12H5.5M11 6l-6 6 6 6"/>',
    "arrow-right": '<path d="M5 12h13.5M13 6l6 6-6 6"/>',
    "arrow-up": '<path d="M12 19V5.5M6 11l6-6 6 6"/>',
    "refresh": '<path d="M19 12a7 7 0 0 1-12.6 4.2"/><path d="M5 12a7 7 0 0 1 12.6-4.2"/><path d="M18 3.5v4.5h-4.5M6 20.5V16h4.5"/>',
    "apps": '<rect x="4.5" y="4.5" width="6" height="6" rx="1.5"/><rect x="13.5" y="4.5" width="6" height="6" rx="1.5"/>'
            '<rect x="4.5" y="13.5" width="6" height="6" rx="1.5"/><rect x="13.5" y="13.5" width="6" height="6" rx="1.5"/>',
    "taskview": '<rect x="3.5" y="7" width="11" height="10" rx="1.5"/><path d="M17.5 8.5h1.5a1.5 1.5 0 0 1 1.5 1.5v4a1.5 1.5 0 0 1-1.5 1.5h-1.5"/>',
    "desktops": '<rect x="3.5" y="5" width="7.5" height="6" rx="1.2"/><rect x="13" y="5" width="7.5" height="6" rx="1.2"/>'
                '<rect x="3.5" y="13" width="7.5" height="6" rx="1.2"/><path d="M16.75 14v4M14.75 16h4"/>',
    "pin": '<path d="M9 4.5h6l-1 5 3 3H7l3-3z"/><path d="M12 12.5v7"/>',
    "unpin": '<path d="M9 4.5h6l-1 5 3 3H7l3-3z"/><path d="M12 12.5v7"/><path d="M4 4l16 16"/>',
    "star": '<path d="M12 4l2.4 5 5.4.6-4 3.7 1.1 5.4L12 16l-4.9 2.7 1.1-5.4-4-3.7 5.4-.6z"/>',
    "dots": '<path d="M6 12h.01M12 12h.01M18 12h.01" stroke-width="2.6"/>',
    "menu": '<path d="M5 7h14M5 12h14M5 17h14"/>',
    "eye": '<path d="M2.5 12S6 5.5 12 5.5 21.5 12 21.5 12 18 18.5 12 18.5 2.5 12 2.5 12z"/><circle cx="12" cy="12" r="2.8"/>',
    "eye-off": '<path d="M9.5 6A9.3 9.3 0 0 1 12 5.5c6 0 9.5 6.5 9.5 6.5a16 16 0 0 1-2.4 3.2"/><path d="M6.4 7.6A15 15 0 0 0 2.5 12S6 18.5 12 18.5a9 9 0 0 0 4.4-1.1"/><path d="M4 4l16 16"/>',
    "list": '<path d="M9 6.5h11M9 12h11M9 17.5h11"/><path d="M4.5 6.5h.01M4.5 12h.01M4.5 17.5h.01" stroke-width="2.4"/>',
    "grid": '<rect x="4" y="4" width="7" height="7" rx="1"/><rect x="13" y="4" width="7" height="7" rx="1"/><rect x="4" y="13" width="7" height="7" rx="1"/><rect x="13" y="13" width="7" height="7" rx="1"/>',
    "sort": '<path d="M7 4.5v15M4 16.5l3 3 3-3M17 19.5v-15M14 7.5l3-3 3 3"/>',
    # --- network / audio ---------------------------------------------------
    "wifi-4": '<path d="M2.8 9a13.5 13.5 0 0 1 18.4 0"/><path d="M5.8 12.3a9 9 0 0 1 12.4 0"/><path d="M8.8 15.5a4.6 4.6 0 0 1 6.4 0"/><path d="M12 19h.01" stroke-width="2.6"/>',
    "wifi-3": '<path d="M2.8 9a13.5 13.5 0 0 1 18.4 0" opacity=".3"/><path d="M5.8 12.3a9 9 0 0 1 12.4 0"/><path d="M8.8 15.5a4.6 4.6 0 0 1 6.4 0"/><path d="M12 19h.01" stroke-width="2.6"/>',
    "wifi-2": '<path d="M2.8 9a13.5 13.5 0 0 1 18.4 0" opacity=".3"/><path d="M5.8 12.3a9 9 0 0 1 12.4 0" opacity=".3"/><path d="M8.8 15.5a4.6 4.6 0 0 1 6.4 0"/><path d="M12 19h.01" stroke-width="2.6"/>',
    "wifi-1": '<path d="M2.8 9a13.5 13.5 0 0 1 18.4 0" opacity=".3"/><path d="M5.8 12.3a9 9 0 0 1 12.4 0" opacity=".3"/><path d="M8.8 15.5a4.6 4.6 0 0 1 6.4 0" opacity=".3"/><path d="M12 19h.01" stroke-width="2.6"/>',
    "wifi-off": '<path d="M2.8 9a13.5 13.5 0 0 1 4-2.6M10.5 5.1A13.5 13.5 0 0 1 21.2 9"/><path d="M5.8 12.3a9 9 0 0 1 3-2"/><path d="M8.8 15.5a4.6 4.6 0 0 1 6.4 0"/><path d="M12 19h.01" stroke-width="2.6"/><path d="M4 4l16 16"/>',
    "wifi-lock": '<path d="M2.8 9a13.5 13.5 0 0 1 18.4 0"/><path d="M5.8 12.3a9 9 0 0 1 8-2.4"/><path d="M8.8 15.5a4.6 4.6 0 0 1 3.6-1.3"/><rect x="15" y="15" width="6" height="5" rx="1"/><path d="M16.5 15v-1.3a1.5 1.5 0 0 1 3 0V15"/>',
    "ethernet": '<rect x="4" y="5" width="16" height="11" rx="2"/><path d="M8 16v3.5h8V16M8.5 9.5v2M11 9.5v2M13.5 9.5v2M16 9.5v2"/>',
    "network-off": '<rect x="4" y="5" width="16" height="11" rx="2"/><path d="M8 16v3.5h8V16"/><path d="M9.5 8.5l5 5M14.5 8.5l-5 5"/>',
    "globe": '<circle cx="12" cy="12" r="8.5"/><path d="M3.5 12h17M12 3.5c2.5 2.3 3.6 5.2 3.6 8.5s-1.1 6.2-3.6 8.5c-2.5-2.3-3.6-5.2-3.6-8.5S9.5 5.8 12 3.5z"/>',
    "volume-high": '<path d="M4 9.5h3.5L12 5.5v13l-4.5-4H4z"/><path d="M15.5 9a4 4 0 0 1 0 6M18 6.5a7.5 7.5 0 0 1 0 11"/>',
    "volume-medium": '<path d="M4 9.5h3.5L12 5.5v13l-4.5-4H4z"/><path d="M15.5 9a4 4 0 0 1 0 6"/>',
    "volume-low": '<path d="M4 9.5h3.5L12 5.5v13l-4.5-4H4z"/>',
    "volume-mute": '<path d="M4 9.5h3.5L12 5.5v13l-4.5-4H4z"/><path d="M15.5 9.5l5 5M20.5 9.5l-5 5"/>',
    "speaker": '<rect x="6" y="3.5" width="12" height="17" rx="2"/><circle cx="12" cy="14" r="3"/><path d="M12 7.5h.01" stroke-width="2.4"/>',
    "headphones": '<path d="M4.5 16v-3a7.5 7.5 0 0 1 15 0v3"/><rect x="3.5" y="14.5" width="4" height="5.5" rx="1.5"/><rect x="16.5" y="14.5" width="4" height="5.5" rx="1.5"/>',
    "battery": '<rect x="3.5" y="7.5" width="15" height="9" rx="2"/><path d="M21 10.5v3"/><path d="M6.5 10.5v3M9.5 10.5v3" />',
    "display": '<rect x="3" y="4.5" width="18" height="12" rx="2"/><path d="M9 20h6M12 16.5V20"/>',
    "keyboard": '<rect x="2.5" y="6" width="19" height="12" rx="2"/><path d="M6 10h.01M9.5 10h.01M13 10h.01M16.5 10h.01M6.5 14h11" stroke-width="1.9"/>',
    "mouse": '<rect x="6.5" y="3.5" width="11" height="17" rx="5.5"/><path d="M12 7v3"/>',
    "palette": '<path d="M12 3.5a8.5 8.5 0 0 0 0 17c1.3 0 1.8-.9 1.4-1.9-.5-1.1.2-2.1 1.4-2.1H17a3.5 3.5 0 0 0 3.5-3.5c0-5.2-3.8-9.5-8.5-9.5z"/><path d="M8 10.5h.01M11 7.5h.01M15 8h.01" stroke-width="2.4"/>',
    "image": '<rect x="3.5" y="5" width="17" height="14" rx="2"/><circle cx="9" cy="10" r="1.6"/><path d="M20.5 15.5l-4.5-4.5-8.5 8"/>',
    "user": '<circle cx="12" cy="8.5" r="3.8"/><path d="M4.5 20a7.5 7.5 0 0 1 15 0"/>',
    "users": '<circle cx="9" cy="9" r="3.3"/><path d="M3 19.5a6 6 0 0 1 12 0"/><path d="M15 5.9a3.3 3.3 0 0 1 0 6.3M17 13.8a6 6 0 0 1 4 5.7"/>',
    "cpu": '<rect x="6" y="6" width="12" height="12" rx="2"/><rect x="9.5" y="9.5" width="5" height="5" rx=".8"/><path d="M9.5 3v3M14.5 3v3M9.5 18v3M14.5 18v3M3 9.5h3M3 14.5h3M18 9.5h3M18 14.5h3"/>',
    "memory": '<rect x="3" y="7" width="18" height="9" rx="1.5"/><path d="M7 10.5v2M11 10.5v2M15 10.5v2M5.5 16v2.5M9 16v2.5M12.5 16v2.5M16 16v2.5"/>',
    "harddisk": '<rect x="3.5" y="5" width="17" height="14" rx="2.5"/><path d="M3.5 13.5h17"/><path d="M16.5 16.3h.01M13.5 16.3h.01" stroke-width="2.2"/>',
    "activity": '<path d="M3 12h4l2.5-6 5 12 2.5-6h4"/>',
    "terminal": '<rect x="3" y="4.5" width="18" height="15" rx="2"/><path d="M7 9.5l3 2.5-3 2.5M12.5 15h4.5"/>',
    "code": '<path d="M9 7l-5 5 5 5M15 7l5 5-5 5"/>',
    # --- files ----------------------------------------------------------------
    "folder": '<path d="M3.5 7.5A1.5 1.5 0 0 1 5 6h4.2l2 2H19a1.5 1.5 0 0 1 1.5 1.5v8A1.5 1.5 0 0 1 19 19H5a1.5 1.5 0 0 1-1.5-1.5z"/>',
    "folder-plus": '<path d="M3.5 7.5A1.5 1.5 0 0 1 5 6h4.2l2 2H19a1.5 1.5 0 0 1 1.5 1.5v8A1.5 1.5 0 0 1 19 19H5a1.5 1.5 0 0 1-1.5-1.5z"/><path d="M12 11v5M9.5 13.5h5"/>',
    "file": '<path d="M14 3.5H7.5A1.5 1.5 0 0 0 6 5v14a1.5 1.5 0 0 0 1.5 1.5h9A1.5 1.5 0 0 0 18 19V7.5z"/><path d="M14 3.5v4h4"/>',
    "file-plus": '<path d="M14 3.5H7.5A1.5 1.5 0 0 0 6 5v14a1.5 1.5 0 0 0 1.5 1.5h9A1.5 1.5 0 0 0 18 19V7.5z"/><path d="M14 3.5v4h4"/><path d="M12 11v6M9 14h6"/>',
    "home": '<path d="M4 11l8-6.5 8 6.5"/><path d="M6 9.5v9.5h4.5v-5h3v5H18V9.5"/>',
    "desktop": '<rect x="3" y="4.5" width="18" height="12" rx="2"/><path d="M9 20h6M12 16.5V20"/>',
    "documents": '<path d="M14 3.5H7.5A1.5 1.5 0 0 0 6 5v14a1.5 1.5 0 0 0 1.5 1.5h9A1.5 1.5 0 0 0 18 19V7.5z"/><path d="M14 3.5v4h4M9 12h6M9 15.5h6"/>',
    "downloads": '<path d="M12 4v11M7.5 10.5L12 15l4.5-4.5"/><path d="M4.5 16.5v2A1.5 1.5 0 0 0 6 20h12a1.5 1.5 0 0 0 1.5-1.5v-2"/>',
    "music": '<path d="M9 17.5V6l10-2v11.5"/><circle cx="6.8" cy="17.5" r="2.2"/><circle cx="16.8" cy="15.5" r="2.2"/>',
    "pictures": '<rect x="3.5" y="5" width="17" height="14" rx="2"/><circle cx="9" cy="10" r="1.6"/><path d="M20.5 15.5l-4.5-4.5-8.5 8"/>',
    "videos": '<rect x="3" y="6" width="13" height="12" rx="2"/><path d="M16 10.5l5-3v9l-5-3"/>',
    "trash": '<path d="M4.5 7h15M9.5 7V4.5h5V7M6.5 7l1 12.5h9l1-12.5"/><path d="M10 11v5M14 11v5"/>',
    "copy": '<rect x="8.5" y="8.5" width="11" height="11" rx="1.5"/><path d="M15.5 8.5V6A1.5 1.5 0 0 0 14 4.5H6A1.5 1.5 0 0 0 4.5 6v8A1.5 1.5 0 0 0 6 15.5h2.5"/>',
    "cut": '<circle cx="6.5" cy="17" r="2.5"/><circle cx="17.5" cy="17" r="2.5"/><path d="M8.5 15.5L18 4.5M15.5 15.5L6 4.5"/>',
    "paste": '<path d="M9 5H7A1.5 1.5 0 0 0 5.5 6.5v12A1.5 1.5 0 0 0 7 20h10a1.5 1.5 0 0 0 1.5-1.5v-12A1.5 1.5 0 0 0 17 5h-2"/><rect x="9" y="3.5" width="6" height="3" rx="1"/>',
    "rename": '<path d="M4.5 19.5l1-4L16 5a2 2 0 0 1 3 3L8.5 18.5z"/><path d="M14 7l3 3"/>',
    "edit": '<path d="M4.5 19.5l1-4L16 5a2 2 0 0 1 3 3L8.5 18.5z"/><path d="M14 7l3 3"/>',
    "open": '<path d="M13 4.5h6.5V11M19.5 4.5L11 13"/><path d="M17 14v4.5A1.5 1.5 0 0 1 15.5 20h-10A1.5 1.5 0 0 1 4 18.5v-10A1.5 1.5 0 0 1 5.5 7H10"/>',
    "save": '<path d="M5 4h11l3.5 3.5V19a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1V5a1 1 0 0 1 1-1z"/><path d="M8 4v4.5h7V4M7.5 20v-6h9v6"/>',
    "undo": '<path d="M9 14.5L4.5 10 9 5.5"/><path d="M4.5 10H15a4.5 4.5 0 0 1 0 9h-3"/>',
    "redo": '<path d="M15 14.5l4.5-4.5L15 5.5"/><path d="M19.5 10H9a4.5 4.5 0 0 0 0 9h3"/>',
    "wrap": '<path d="M4 6.5h16M4 12h13a3 3 0 0 1 0 6h-4M4 17.5h5"/><path d="M14.5 16l-2 2 2 2"/>',
    "text": '<path d="M5 6.5V5h14v1.5M12 5v14M9.5 19h5"/>',
    "properties": '<circle cx="12" cy="12" r="8.5"/><path d="M12 11v5.5"/><path d="M12 7.6v.2" stroke-width="2.2"/>',
    "file-text": '<path d="M14 3.5H7.5A1.5 1.5 0 0 0 6 5v14a1.5 1.5 0 0 0 1.5 1.5h9A1.5 1.5 0 0 0 18 19V7.5z"/><path d="M14 3.5v4h4M9 12h6M9 15.5h4"/>',
    "drive": '<rect x="3.5" y="7" width="17" height="10" rx="2"/><path d="M17 12h.01M14 12h.01" stroke-width="2.2"/>',
}

# --- Application icons (48x48, full colour, original designs) ---------------
APP_HEAD = '<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 48 48">'

APPS = {
    # Files: a folder in warm sand tones
    "meridian-files": '''
  <path d="M5 13a4 4 0 0 1 4-4h9.2a3 3 0 0 1 2.1.9L23 12.6h16a4 4 0 0 1 4 4V37a4 4 0 0 1-4 4H9a4 4 0 0 1-4-4z" fill="#C98A2E"/>
  <path d="M5 19a4 4 0 0 1 4-4h30a4 4 0 0 1 4 4v18a4 4 0 0 1-4 4H9a4 4 0 0 1-4-4z" fill="#E9B04F"/>
  <path d="M5 19a4 4 0 0 1 4-4h30a4 4 0 0 1 4 4v1.2H5z" fill="#F4C978"/>''',
    # Settings: two sliders on a slate tile
    "meridian-settings": '''
  <rect x="4" y="4" width="40" height="40" rx="11" fill="#4A5560"/>
  <path d="M13 17h22M13 31h22" stroke="#9AA6B1" stroke-width="3.2" stroke-linecap="round"/>
  <circle cx="29" cy="17" r="5" fill="#F2F4F5"/>
  <circle cx="19" cy="31" r="5" fill="#5BC7B4"/>''',
    # Terminal: prompt on a near-black tile
    "meridian-terminal": '''
  <rect x="4" y="6" width="40" height="36" rx="9" fill="#1F2426"/>
  <rect x="4" y="6" width="40" height="8" rx="4" fill="#2C3336"/>
  <path d="M13 22l6 5-6 5" fill="none" stroke="#5BC7B4" stroke-width="3.2" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M23 33h11" stroke="#E7ECEB" stroke-width="3.2" stroke-linecap="round"/>''',
    # System monitor: pulse line over bars
    "meridian-monitor": '''
  <rect x="4" y="4" width="40" height="40" rx="11" fill="#14695E"/>
  <rect x="11" y="27" width="5" height="9" rx="1.5" fill="#2E9283"/>
  <rect x="21.5" y="21" width="5" height="15" rx="1.5" fill="#2E9283"/>
  <rect x="32" y="24" width="5" height="12" rx="1.5" fill="#2E9283"/>
  <path d="M9 24h7l3.5-8 6 15 3.5-7h10" fill="none" stroke="#E9F7F4" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>''',
    # Text editor: a sheet with lines and a nib
    "meridian-editor": '''
  <path d="M11 4h18l10 10v26a4 4 0 0 1-4 4H11a4 4 0 0 1-4-4V8a4 4 0 0 1 4-4z" fill="#F1F3F2"/>
  <path d="M29 4l10 10h-7a3 3 0 0 1-3-3z" fill="#C9D1CE"/>
  <path d="M13 21h16M13 27h12M13 33h8" stroke="#8E9A96" stroke-width="2.6" stroke-linecap="round"/>
  <path d="M27 38l2-6 10-10 4 4-10 10z" fill="#2E9283"/>
  <path d="M27 38l2-6 4 4z" fill="#1A4F48"/>''',
    # Installer: drive with downward arrow
    "meridian-installer": '''
  <rect x="5" y="24" width="38" height="17" rx="5" fill="#4A5560"/>
  <circle cx="35" cy="32.5" r="2.2" fill="#5BC7B4"/>
  <path d="M24 5v16" stroke="#2E9283" stroke-width="4" stroke-linecap="round"/>
  <path d="M16.5 14.5L24 22l7.5-7.5" fill="none" stroke="#2E9283" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"/>''',
    # Generic application fallback
    "meridian-app": '''
  <rect x="4" y="4" width="40" height="40" rx="11" fill="#6B7A85"/>
  <rect x="13" y="13" width="9" height="9" rx="2.5" fill="#E8EDEF"/>
  <rect x="26" y="13" width="9" height="9" rx="2.5" fill="#C3CDD2"/>
  <rect x="13" y="26" width="9" height="9" rx="2.5" fill="#C3CDD2"/>
  <rect x="26" y="26" width="9" height="9" rx="2.5" fill="#E8EDEF"/>''',
    # OS mark: a circle crossed by a meridian arc (the distro's logo)
    "customos-logo": '''
  <circle cx="24" cy="24" r="19" fill="#14695E"/>
  <path d="M24 5c-7 5-10.5 11.6-10.5 19S17 38 24 43" fill="none" stroke="#E9F7F4" stroke-width="3.4" stroke-linecap="round"/>
  <path d="M24 5c7 5 10.5 11.6 10.5 19S31 38 24 43" fill="none" stroke="#5BC7B4" stroke-width="3.4" stroke-linecap="round"/>
  <path d="M7 24h34" stroke="#E9F7F4" stroke-width="3.4" stroke-linecap="round" opacity=".55"/>''',
}


def main():
    sym_dir = os.path.join(HERE, "symbolic")
    app_dir = os.path.join(HERE, "apps")
    os.makedirs(sym_dir, exist_ok=True)
    os.makedirs(app_dir, exist_ok=True)
    for name, body in SYMBOLIC.items():
        with open(os.path.join(sym_dir, name + ".svg"), "w") as f:
            f.write(SYM_HEAD + body + "</svg>\n")
    for name, body in APPS.items():
        with open(os.path.join(app_dir, name + ".svg"), "w") as f:
            f.write(APP_HEAD + body + "\n</svg>\n")
    print(f"wrote {len(SYMBOLIC)} symbolic and {len(APPS)} application icons")


if __name__ == "__main__":
    main()
