pragma Singleton
// Screen recording through gpu-screen-recorder: toggle() starts recording the focused monitor
// (with desktop audio) into Config.recorder.dir, toggle() again stops and saves it.
import Quickshell
import Quickshell.Io
import QtQuick
import qs.services

Singleton {
    id: root

    readonly property bool recording: proc.running
    property string lastFile: ""

    function toggle(): void {
        if (proc.running) {
            proc.signal(2); // SIGINT makes gpu-screen-recorder finish and save the file
            return;
        }
        const stamp = Qt.formatDateTime(new Date(), "yyyy-MM-dd_HH-mm-ss");
        lastFile = `${Config.expandHome(Config.recorder.dir)}/recording_${stamp}.mp4`;
        proc.command = ["sh", "-c", 'mkdir -p "$(dirname "$1")" && exec gpu-screen-recorder -w "$2" -f 60 -a default_output -o "$1"', "sh", lastFile, Compositor.focusedScreen || "screen"];
        proc.running = true;
    }

    Process {
        id: proc
        stderr: StdioCollector {
            id: err
        }
        onExited: code => {
            if (code !== 0)
                console.warn("gpu-screen-recorder exited with", code, err.text);
        }
    }
}
