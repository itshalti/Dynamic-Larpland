pragma Singleton
// Session lock state and PAM authentication for modules/lock. The typed password lives here so
// every screen's lock surface shows the same masked field.
import Quickshell
import Quickshell.Services.Pam
import QtQuick

Singleton {
    id: root

    property bool locked: false
    // What the user has typed so far (never shown, only its length).
    property string input: ""
    property string error: ""
    readonly property bool checking: pam.active

    // Emitted on a wrong password (the field shakes).
    signal failed

    function lock(): void {
        input = "";
        error = "";
        locked = true;
    }

    function submit(): void {
        if (pam.active || input === "")
            return;
        error = "";
        pam.start();
    }

    PamContext {
        id: pam

        // passwd = plain system-auth (pam_unix + faillock), no tty checks.
        config: "passwd"

        onResponseRequiredChanged: {
            if (responseRequired)
                respond(root.input);
        }

        onCompleted: result => {
            root.input = "";
            if (result === PamResult.Success) {
                root.locked = false;
                return;
            }
            root.error = "Wrong password";
            root.failed();
        }

        onError: error => {
            root.input = "";
            root.error = "Can't check the password";
            root.failed();
            console.warn("PAM error:", PamError.toString(error));
        }
    }
}
