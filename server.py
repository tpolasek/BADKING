#!/usr/bin/env python3
"""Simple REST server wrapping the badking text adventure for LLM play."""

import os
import pty
import select
import subprocess
import time
from flask import Flask, request, jsonify

app = Flask(__name__)

BIN = os.path.join(os.path.dirname(os.path.abspath(__file__)), "badking")


class Game:
    def __init__(self):
        self.master_fd = None
        self.pid = None
        self.last_output = ""
        self._start()

    def _start(self):
        self.pid, self.master_fd = pty.fork()
        if self.pid == 0:
            os.execv(BIN, [BIN])
        # Read initial screen
        self.last_output = self._read()

    def _read(self, timeout=0.5):
        buf = b""
        while True:
            r, _, _ = select.select([self.master_fd], [], [], timeout)
            if not r:
                break
            try:
                chunk = os.read(self.master_fd, 4096)
            except OSError:
                break
            if not chunk:
                break
            buf += chunk
        return buf.decode("utf-8", errors="replace")

    def send(self, char):
        if len(char) != 1:
            return {"error": "Command must be a single character"}, 400
        os.write(self.master_fd, char.encode())
        time.sleep(0.3)  # wait for game to process
        self.last_output = self._read()
        return {"output": self.last_output}

    def state(self):
        return {"output": self.last_output}

    def restart(self):
        if self.master_fd is not None:
            try:
                os.close(self.master_fd)
            except OSError:
                pass
        if self.pid is not None:
            try:
                os.kill(self.pid, 9)
                os.waitpid(self.pid, 0)
            except (OSError, ChildProcessError):
                pass
        self._start()


game = Game()


@app.route("/state", methods=["GET"])
def get_state():
    """Get the current/last screen state."""
    return jsonify(game.state())


@app.route("/command", methods=["POST"])
def send_command():
    """Send a single character command. Body: {"char": "x"}"""
    data = request.get_json()
    if not data or "char" not in data:
        return jsonify({"error": "Missing 'char' in JSON body"}), 400
    result = game.send(data["char"])
    if isinstance(result, tuple):
        return jsonify(result[0]), result[1]
    return jsonify(result)


@app.route("/restart", methods=["POST"])
def restart():
    """Restart the game."""
    game.restart()
    return jsonify(game.state())


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
