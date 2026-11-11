pragma Singleton
// MPRIS players. `active` is the user-selected player, else the one playing, else the first.
import Quickshell
import Quickshell.Services.Mpris
import QtQuick

Singleton {
    id: root

    readonly property var players: Mpris.players.values
    property MprisPlayer selected: null

    readonly property MprisPlayer active: {
        const list = root.players;
        if (selected && list.includes(selected))
            return selected;
        return list.find(p => p.isPlaying) ?? list[0] ?? null;
    }
    readonly property bool playing: active?.isPlaying ?? false

    function cyclePlayer(): void {
        const list = root.players;
        if (list.length < 2)
            return;
        selected = list[(list.indexOf(active) + 1) % list.length];
    }

    function formatTime(seconds: real): string {
        if (!isFinite(seconds) || seconds < 0)
            seconds = 0;
        const m = Math.floor(seconds / 60);
        const s = Math.floor(seconds % 60);
        return `${m}:${s < 10 ? "0" : ""}${s}`;
    }

    // Icon of the app behind a player (Spotify, Firefox…), for the badge on the cover.
    function appIcon(player: MprisPlayer): string {
        if (!player)
            return "";
        const entry = DesktopEntries.heuristicLookup(player.desktopEntry || player.identity);
        const name = entry?.icon || player.desktopEntry || player.identity.toLowerCase();
        if (name.startsWith("/"))
            return "file://" + name;
        return Quickshell.hasThemeIcon(name) ? Quickshell.iconPath(name) : "";
    }

    // MPRIS doesn't push position updates; poke the binding while playing.
    Timer {
        running: root.playing
        interval: 1000
        repeat: true
        onTriggered: root.active.positionChanged()
    }
}
