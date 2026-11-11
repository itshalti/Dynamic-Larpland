pragma Singleton
// Installed .desktop applications and fuzzy search over them.
import Quickshell
import QtQuick

Singleton {
    id: root

    readonly property var list: DesktopEntries.applications.values.filter(a => !a.noDisplay).sort((a, b) => a.name.localeCompare(b.name))

    function score(entry: DesktopEntry, q: string): int {
        const name = entry.name.toLowerCase();
        if (name === q)
            return 1000;
        if (name.startsWith(q))
            return 800 - name.length;
        if (name.split(/[\s\-_.]+/).some(w => w.startsWith(q)))
            return 600 - name.length;
        if (name.includes(q))
            return 400 - name.length;
        const extra = `${entry.genericName} ${entry.keywords} ${entry.id}`.toLowerCase();
        if (extra.includes(q))
            return 200;
        // Subsequence match: "vsc" -> "VSCodium"
        let i = 0;
        for (const ch of name) {
            if (ch === q[i])
                i++;
            if (i === q.length)
                return Math.max(1, 100 - name.length);
        }
        return 0;
    }

    function query(text: string): var {
        const q = text.trim().toLowerCase();
        if (!q)
            return list;
        return list.map(e => ({
                    e,
                    s: score(e, q)
                })).filter(x => x.s > 0).sort((a, b) => b.s - a.s).map(x => x.e);
    }

    function subtitle(entry: DesktopEntry): string {
        return entry.genericName || entry.comment || "";
    }

    function icon(entry: DesktopEntry): string {
        return Quickshell.iconPath(entry.icon, "application-x-executable");
    }

    function launch(entry: DesktopEntry): void {
        entry.execute();
    }
}
