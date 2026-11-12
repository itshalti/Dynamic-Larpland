//@ pragma UseQApplication
// Entry point: a wallpaper and an island per connected screen, plus a single launcher and the lock screen.
import Quickshell
import qs.modules.island
import qs.modules.launcher
import qs.modules.wallpaper
import qs.modules.lock

ShellRoot {
    Variants {
        model: Quickshell.screens

        Background {}
    }

    Variants {
        model: Quickshell.screens

        Island {}
    }

    Launcher {}

    LockScreen {}
}
