//@ pragma UseQApplication
// Entry point: one island per connected screen.
import Quickshell
import qs.modules.island

ShellRoot {
    Variants {
        model: Quickshell.screens

        Island {}
    }
}
