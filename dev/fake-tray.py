#!/usr/bin/env python3
"""Demo tray item for the nested dev session: a StatusNotifierItem ("Demo Tray") with a themed icon and a
DBusMenu (two actions, a checkbox, a submenu). Run it on the nested session bus:

    DBUS_SESSION_BUS_ADDRESS=... dev/fake-tray.py

Clicked menu entries are printed to stderr.
"""

import os
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

ITEM_PATH = "/StatusNotifierItem"
MENU_PATH = "/MenuBar"
SNI = "org.kde.StatusNotifierItem"
DBUSMENU = "com.canonical.dbusmenu"

# id -> (properties, children ids)
MENU = {
    0: ({"children-display": "submenu"}, [1, 2, 3, 4, 5]),
    1: ({"label": "Show window"}, []),
    2: ({"label": "Do not disturb", "toggle-type": "checkmark", "toggle-state": 0}, []),
    3: ({"type": "separator"}, []),
    4: ({"label": "More", "children-display": "submenu"}, [6, 7]),
    5: ({"label": "Quit demo"}, []),
    6: ({"label": "About"}, []),
    7: ({"label": "Preferences"}, []),
}


def layout(node_id, depth):
    props, children = MENU[node_id]
    kids = []
    if depth != 0:
        kids = [dbus.Struct(layout(c, depth - 1), variant_level=1) for c in children]
    return (dbus.Int32(node_id), dbus.Dictionary(props, signature="sv"), dbus.Array(kids, signature="v"))


class Menu(dbus.service.Object):
    @dbus.service.method(DBUSMENU, in_signature="iias", out_signature="u(ia{sv}av)")
    def GetLayout(self, parent_id, depth, _props):
        return dbus.UInt32(1), layout(parent_id, depth)

    @dbus.service.method(DBUSMENU, in_signature="aias", out_signature="a(ia{sv})")
    def GetGroupProperties(self, ids, _props):
        return [(dbus.Int32(i), dbus.Dictionary(MENU[i][0], signature="sv")) for i in ids if i in MENU]

    @dbus.service.method(DBUSMENU, in_signature="isvu", out_signature="")
    def Event(self, item_id, event_id, _data, _timestamp):
        if event_id == "clicked":
            print(f"fake-tray: clicked {MENU.get(item_id, ({}, []))[0].get('label', item_id)}", file=sys.stderr)
            if item_id == 2:
                props = MENU[2][0]
                props["toggle-state"] = 1 - props["toggle-state"]
                self.LayoutUpdated(dbus.UInt32(2), dbus.Int32(0))
            if item_id == 5:
                loop.quit()

    @dbus.service.method(DBUSMENU, in_signature="i", out_signature="b")
    def AboutToShow(self, _item_id):
        return False

    @dbus.service.signal(DBUSMENU, signature="ui")
    def LayoutUpdated(self, revision, parent):
        pass

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature="ss", out_signature="v")
    def Get(self, _iface, prop):
        return {"Version": dbus.UInt32(3), "Status": "normal"}.get(prop, "")

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature="s", out_signature="a{sv}")
    def GetAll(self, _iface):
        return {"Version": dbus.UInt32(3), "Status": "normal"}


class Item(dbus.service.Object):
    PROPS = {
        "Category": "ApplicationStatus",
        "Id": "demo-tray",
        "Title": "Demo Tray",
        "Status": "Active",
        "IconName": "audio-volume-high",
        "ItemIsMenu": False,
        "Menu": dbus.ObjectPath(MENU_PATH),
    }

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature="ss", out_signature="v")
    def Get(self, _iface, prop):
        return self.PROPS[prop]

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature="s", out_signature="a{sv}")
    def GetAll(self, _iface):
        return self.PROPS

    @dbus.service.method(SNI, in_signature="ii", out_signature="")
    def Activate(self, _x, _y):
        print("fake-tray: activated", file=sys.stderr)

    @dbus.service.method(SNI, in_signature="ii", out_signature="")
    def SecondaryActivate(self, _x, _y):
        pass

    @dbus.service.method(SNI, in_signature="ii", out_signature="")
    def ContextMenu(self, _x, _y):
        pass


DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
name = dbus.service.BusName(f"org.kde.StatusNotifierItem-{os.getpid()}-1", bus)
item = Item(bus, ITEM_PATH)
menu = Menu(bus, MENU_PATH)
watcher = bus.get_object("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher")
watcher.RegisterStatusNotifierItem(name.get_name(), dbus_interface="org.kde.StatusNotifierWatcher")
print("fake-tray: registered", file=sys.stderr)
loop = GLib.MainLoop()
loop.run()
