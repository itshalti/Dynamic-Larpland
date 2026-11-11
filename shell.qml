//@ pragma UseQApplication
// Entry point: one island per connected screen, plus a single launcher.
import Quickshell
import qs.modules.island
import qs.modules.launcher

ShellRoot {
    Variants {
        model: Quickshell.screens

        Island {}
    }

    Launcher {}
}
