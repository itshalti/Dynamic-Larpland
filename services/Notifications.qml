pragma Singleton
// The notification daemon (org.freedesktop.Notifications) plus a persisted history.
// History entries are plain objects so they survive restarts; the live Notification
// objects are kept on the side for actions and dismissal while they still exist.
import Quickshell
import Quickshell.Io
import Quickshell.Services.Notifications
import QtQuick
import qs.services

Singleton {
    id: root

    // Newest first: { key, appName, icon, image, summary, body, time }
    property var history: []
    property var live: ({})

    signal posted(var entry)

    function resolveIcon(n: Notification): string {
        const raw = n.appIcon || "";
        if (raw.startsWith("/"))
            return "file://" + raw;
        if (raw.startsWith("file://") || raw.startsWith("image://"))
            return raw;
        const entry = DesktopEntries.heuristicLookup(n.desktopEntry || n.appName);
        for (const name of [raw, entry?.icon, n.desktopEntry, n.appName.toLowerCase()]) {
            if (!name)
                continue;
            if (Quickshell.hasThemeIcon(name))
                return Quickshell.iconPath(name);
        }
        return "";
    }

    // Theme-icon URLs for icons that don't exist render as a checkerboard; drop those.
    function validImage(image: string): string {
        if (!image)
            return "";
        if (image.startsWith("image://icon/"))
            return Quickshell.hasThemeIcon(image.slice(13).split("?")[0]) ? image : "";
        return image;
    }

    function add(n: Notification): void {
        n.tracked = true;
        const entry = {
            key: `${Date.now()}-${n.id}`,
            appName: n.appName || "",
            icon: resolveIcon(n),
            image: validImage(n.image),
            summary: n.summary || "",
            body: n.body || "",
            time: Date.now()
        };
        live[entry.key] = n;
        n.closed.connect(() => delete root.live[entry.key]);

        const list = [entry, ...history];
        while (list.length > Config.notifications.historyLimit)
            dismissLive(list.pop().key);
        history = list;
        save();
        posted(entry);
    }

    // Click on an entry: run the app's default action if it still exists, then drop it.
    function activate(key: string): void {
        const n = live[key];
        const action = n?.actions.find(a => a.identifier === "default");
        if (action)
            action.invoke();
        remove(key);
    }

    function remove(key: string): void {
        dismissLive(key);
        history = history.filter(e => e.key !== key);
        save();
    }

    function clear(): void {
        for (const e of history)
            dismissLive(e.key);
        history = [];
        save();
    }

    function dismissLive(key: string): void {
        const n = live[key];
        if (!n)
            return;
        delete live[key];
        n.dismiss();
    }

    function ago(time: real, now: date): string {
        const minutes = Math.floor((now.getTime() - time) / 60000);
        if (minutes < 1)
            return "now";
        if (minutes < 60)
            return `${minutes}m`;
        if (minutes < 60 * 24)
            return `${Math.floor(minutes / 60)}h`;
        return Qt.formatDate(new Date(time), "d MMM");
    }

    function save(): void {
        store.setText(JSON.stringify(history));
    }

    NotificationServer {
        bodySupported: true
        imageSupported: true
        actionsSupported: true
        persistenceSupported: true
        onNotification: n => root.add(n)
    }

    FileView {
        id: store
        path: Config.stateDir + "/notifications.json"
        printErrors: false
        onLoaded: {
            try {
                const saved = JSON.parse(text());
                if (Array.isArray(saved))
                    root.history = saved;
            } catch (e) {
                console.warn("notifications: bad history file", e);
            }
        }
    }
}
