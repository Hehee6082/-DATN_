function sendCommand(cmd) {
    fetch('/send_command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command: cmd })
    }).then(res => console.log("Gửi lệnh:", cmd));
}
