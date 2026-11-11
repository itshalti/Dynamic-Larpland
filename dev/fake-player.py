#!/usr/bin/env python3
"""A fake MPRIS media player for the nested dev session (and for recording demo media).

The nested session has its own D-Bus, so real players on the host aren't visible there.
This exposes "Demo Player" with a few made-up tracks and generated cover art; play/pause,
next/previous and seeking all work. Run inside the session's bus, e.g. from dev/hyprland-nested.lua
(ALT+P) or: DBUS_SESSION_BUS_ADDRESS=... dev/fake-player.py
"""

import os
import sys
import time

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

ROOT = "/org/mpris/MediaPlayer2"
IFACE_ROOT = "org.mpris.MediaPlayer2"
IFACE_PLAYER = "org.mpris.MediaPlayer2.Player"
IFACE_PROPS = "org.freedesktop.DBus.Properties"

TRACKS = [
    ("Neon Harbor", "Midnight Arcade", "Afterglow", 214, ((20, 184, 166), (88, 28, 135))),
    ("Paper Satellites", "Lumen Drift", "Orbit Songs", 187, ((249, 115, 22), (190, 24, 93))),
    ("Glass Garden", "Velvet Static", "Greenhouse", 243, ((132, 204, 22), (14, 116, 144))),
]


def make_cover(path, colors):
    """Diagonal two-colour gradient with a soft circle — no copyrighted art involved."""
    from PIL import Image, ImageDraw, ImageFilter

    size = 512
    (r1, g1, b1), (r2, g2, b2) = colors
    img = Image.new("RGB", (size, size))
    px = img.load()
    for y in range(size):
        for x in range(size):
            t = (x + y) / (2 * size - 2)
            px[x, y] = (int(r1 + (r2 - r1) * t), int(g1 + (g2 - g1) * t), int(b1 + (b2 - b1) * t))
    glow = Image.new("L", (size, size), 0)
    ImageDraw.Draw(glow).ellipse((120, 90, 400, 370), fill=170)
    glow = glow.filter(ImageFilter.GaussianBlur(40))
    img.paste((255, 255, 255), (0, 0), glow)
    img.save(path)


class Player(dbus.service.Object):
    def __init__(self, bus, cover_dir):
        super().__init__(bus, ROOT)
        self.covers = []
        for i, track in enumerate(TRACKS):
            path = os.path.join(cover_dir, f"cover-{i}.png")
            if not os.path.exists(path):
                make_cover(path, track[4])
            self.covers.append("file://" + path)
        self.index = 0
        self.playing = True
        self.offset = 0.0  # seconds into the track when last (re)started/paused
        self.started = time.monotonic()

    # --- state helpers ---
    def position(self):
        pos = self.offset + (time.monotonic() - self.started if self.playing else 0)
        length = TRACKS[self.index][3]
        if pos >= length:
            self.set_track((self.index + 1) % len(TRACKS))
            return 0.0
        return pos

    def metadata(self):
        title, artist, album, length, _ = TRACKS[self.index]
        return dbus.Dictionary({
            "mpris:trackid": dbus.ObjectPath(f"/demo/track/{self.index}"),
            "mpris:length": dbus.Int64(length * 1_000_000),
            "mpris:artUrl": self.covers[self.index],
            "xesam:title": title,
            "xesam:artist": dbus.Array([artist], signature="s"),
            "xesam:album": album,
        }, signature="sv")

    def player_props(self):
        return {
            "PlaybackStatus": "Playing" if self.playing else "Paused",
            "LoopStatus": "None",
            "Rate": 1.0,
            "Shuffle": False,
            "Metadata": self.metadata(),
            "Volume": 1.0,
            "Position": dbus.Int64(int(self.position() * 1_000_000)),
            "MinimumRate": 1.0,
            "MaximumRate": 1.0,
            "CanGoNext": True,
            "CanGoPrevious": True,
            "CanPlay": True,
            "CanPause": True,
            "CanSeek": True,
            "CanControl": True,
        }

    def root_props(self):
        return {
            "CanQuit": False,
            "CanRaise": False,
            "HasTrackList": False,
            "Identity": "Demo Player",
            "DesktopEntry": "",
            "SupportedUriSchemes": dbus.Array([], signature="s"),
            "SupportedMimeTypes": dbus.Array([], signature="s"),
        }

    def changed(self, *names):
        props = self.player_props()
        self.PropertiesChanged(IFACE_PLAYER, {n: props[n] for n in names}, [])

    def set_track(self, index):
        self.index = index
        self.offset = 0.0
        self.started = time.monotonic()
        self.changed("Metadata", "PlaybackStatus")
        self.Seeked(dbus.Int64(0))

    def seek_to(self, seconds):
        length = TRACKS[self.index][3]
        self.offset = max(0.0, min(seconds, length - 1))
        self.started = time.monotonic()
        self.Seeked(dbus.Int64(int(self.offset * 1_000_000)))

    # --- org.freedesktop.DBus.Properties ---
    @dbus.service.method(IFACE_PROPS, in_signature="ss", out_signature="v")
    def Get(self, interface, name):
        return self.GetAll(interface)[name]

    @dbus.service.method(IFACE_PROPS, in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface):
        if interface == IFACE_PLAYER:
            return self.player_props()
        if interface == IFACE_ROOT:
            return self.root_props()
        return {}

    @dbus.service.method(IFACE_PROPS, in_signature="ssv")
    def Set(self, interface, name, value):
        pass

    @dbus.service.signal(IFACE_PROPS, signature="sa{sv}as")
    def PropertiesChanged(self, interface, changed, invalidated):
        pass

    # --- org.mpris.MediaPlayer2 ---
    @dbus.service.method(IFACE_ROOT)
    def Raise(self):
        pass

    @dbus.service.method(IFACE_ROOT)
    def Quit(self):
        pass

    # --- org.mpris.MediaPlayer2.Player ---
    @dbus.service.signal(IFACE_PLAYER, signature="x")
    def Seeked(self, position):
        pass

    @dbus.service.method(IFACE_PLAYER)
    def Next(self):
        self.set_track((self.index + 1) % len(TRACKS))

    @dbus.service.method(IFACE_PLAYER)
    def Previous(self):
        self.set_track((self.index - 1) % len(TRACKS))

    @dbus.service.method(IFACE_PLAYER)
    def Play(self):
        if not self.playing:
            self.started = time.monotonic()
            self.playing = True
            self.changed("PlaybackStatus")

    @dbus.service.method(IFACE_PLAYER)
    def Pause(self):
        if self.playing:
            self.offset = self.position()
            self.playing = False
            self.changed("PlaybackStatus")

    @dbus.service.method(IFACE_PLAYER)
    def PlayPause(self):
        self.Pause() if self.playing else self.Play()

    @dbus.service.method(IFACE_PLAYER)
    def Stop(self):
        self.Pause()

    @dbus.service.method(IFACE_PLAYER, in_signature="x")
    def Seek(self, offset):
        self.seek_to(self.position() + offset / 1_000_000)

    @dbus.service.method(IFACE_PLAYER, in_signature="ox")
    def SetPosition(self, track_id, position):
        self.seek_to(position / 1_000_000)

    @dbus.service.method(IFACE_PLAYER, in_signature="s")
    def OpenUri(self, uri):
        pass


def main():
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    name = dbus.service.BusName("org.mpris.MediaPlayer2.dynamic_larpland_demo", bus)  # noqa: F841 (keeps the name)
    state = os.environ.get("XDG_STATE_HOME") or os.path.expanduser("~/.local/state")
    cover_dir = os.path.join(state, "dynamic-larpland-demo")
    os.makedirs(cover_dir, exist_ok=True)
    Player(bus, cover_dir)
    print("fake-player: running as org.mpris.MediaPlayer2.dynamic_larpland_demo", file=sys.stderr)
    GLib.MainLoop().run()


if __name__ == "__main__":
    main()
