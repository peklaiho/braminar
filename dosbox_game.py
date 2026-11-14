#!/usr/bin/env python3
"""Run the original Braminar and read its DOS text screen through the debugger."""

import argparse
import errno
import fcntl
import os
from pathlib import Path
import pty
import select
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import termios
import time


class Game:
    def __init__(self, delay=0.2, timeout=10):
        self.delay = delay
        self.timeout = timeout
        self.processes = []
        self.master = None
        self.directory = None
        self.output = bytearray()

    def __enter__(self):
        try:
            self.start()
            return self
        except BaseException:
            self.__exit__(None, None, None)
            raise

    def __exit__(self, *_):
        for process in reversed(self.processes):
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        if self.master is not None:
            os.close(self.master)
        if self.directory is not None:
            self.directory.cleanup()

    def start(self):
        for program in ('Xvfb', 'dosbox-debug', 'xdotool'):
            if shutil.which(program) is None:
                raise RuntimeError(f'Required executable not found: {program}')
        self.directory = tempfile.TemporaryDirectory(prefix='braminar-dosbox-')
        self.path = Path(self.directory.name)
        shutil.copytree(Path(__file__).resolve().parent / 'dos', self.path / 'game')
        config = self.path / 'dosbox.conf'
        config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n'
                          '[cpu]\ncycles=fixed 3000\n[mixer]\nnosound=true\n'
                          '[midi]\nmpu401=none\n[sblaster]\nsbtype=none\n'
                          '[gus]\ngus=false\n[speaker]\npcspeaker=false\n')
        read_fd, write_fd = os.pipe()
        try:
            with (self.path / 'xvfb.log').open('wb') as log:
                display = subprocess.Popen(
                    ['Xvfb', '-displayfd', str(write_fd), '-screen', '0',
                     '1024x768x24', '-ac', '-nolisten', 'tcp'],
                    pass_fds=(write_fd,), stdout=log, stderr=log)
            self.processes.append(display)
            os.close(write_fd)
            write_fd = None
            if not select.select([read_fd], [], [], self.timeout)[0]:
                raise RuntimeError('Timed out starting the virtual display')
            number = os.read(read_fd, 64).strip()
            if not number.isdigit():
                raise RuntimeError('Virtual display failed: ' +
                                   (self.path / 'xvfb.log').read_text())
        finally:
            os.close(read_fd)
            if write_fd is not None:
                os.close(write_fd)
        self.env = dict(os.environ, DISPLAY=':' + number.decode(),
                        SDL_AUDIODRIVER='dummy', TERM='xterm')
        self.master, slave = pty.openpty()
        try:
            fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 50, 120, 0, 0))
            self.dosbox = subprocess.Popen(
                ['dosbox-debug', '-conf', str(config),
                 '-c', f'mount c {self.path / "game"}', '-c', 'c:',
                 '-c', 'braminar.com'], cwd=self.path, env=self.env,
                stdin=slave, stdout=slave, stderr=slave)
            self.processes.append(self.dosbox)
        finally:
            os.close(slave)
        self.await_output(b'EXEC:Execute braminar.com')
        self.settle()
        deadline = time.monotonic() + self.timeout
        while 'Welcome to:' not in self.screen():
            if time.monotonic() >= deadline:
                raise RuntimeError('Timed out waiting for the Braminar welcome screen')
            self.settle()

    def pump(self, timeout):
        if select.select([self.master], [], [], timeout)[0]:
            try:
                data = os.read(self.master, 65536)
            except OSError as error:
                if error.errno != errno.EIO:
                    raise
                data = b''
            if not data:
                raise RuntimeError('DOSBox exited')
            self.output.extend(data)
            self.output = self.output[-131072:]
        if self.dosbox.poll() is not None:
            raise RuntimeError(f'DOSBox exited with status {self.dosbox.returncode}')

    def await_output(self, marker):
        deadline = time.monotonic() + self.timeout
        while marker not in self.output:
            if time.monotonic() >= deadline:
                raise RuntimeError(f'Timed out waiting for DOSBox: {marker.decode()}')
            self.pump(0.05)

    def settle(self, seconds=None):
        deadline = time.monotonic() + (self.delay if seconds is None else seconds)
        while time.monotonic() < deadline:
            self.pump(min(0.05, max(0, deadline - time.monotonic())))

    def keyboard(self, *arguments):
        subprocess.run(['xdotool', *arguments], env=self.env, check=True,
                       stdout=subprocess.DEVNULL, timeout=self.timeout)

    def dump(self, segment, offset, size):
        path = self.path / 'MEMDUMP.BIN'
        path.unlink(missing_ok=True)
        self.output.clear()
        command = f'MEMDUMPBIN {segment:04X}:{offset:04X} {size:X}'
        os.write(self.master, command.encode('ascii'))
        # Give curses time to consume the command before submitting Enter.
        self.settle(0.05)
        os.write(self.master, b'\n')
        deadline = time.monotonic() + self.timeout
        while not path.exists() or path.stat().st_size != size:
            if time.monotonic() >= deadline:
                raise RuntimeError('Timed out waiting for the video memory dump')
            self.pump(0.05)
        data = path.read_bytes()
        if len(data) != size:
            raise RuntimeError(f'Expected {size} dump bytes, got {len(data)}')
        return data

    def screen(self):
        self.keyboard('key', 'alt+Pause')
        self.settle(0.15)
        try:
            bios = self.dump(0x40, 0, 0x100)
            mode = bios[0x49]
            columns = int.from_bytes(bios[0x4A:0x4C], 'little')
            offset = int.from_bytes(bios[0x4E:0x50], 'little')
            rows = bios[0x84] + 1
            if mode not in (0, 1, 2, 3, 7):
                raise RuntimeError(f'Video mode {mode} is not a text mode')
            if not (1 <= columns <= 160 and 1 <= rows <= 100):
                raise RuntimeError(f'Unexpected text screen size: {columns}x{rows}')
            size = columns * rows * 2
            if offset + size > 0x8000:
                raise RuntimeError('Text screen exceeds video memory')
            data = self.dump(0xB000 if mode == 7 else 0xB800, offset, size)
            lines = []
            for row in range(rows):
                start = row * columns * 2
                characters = data[start:start + columns * 2:2]
                lines.append(characters.replace(b'\0', b' ').decode('cp437').rstrip())
            return '\n'.join(lines).strip('\n')
        finally:
            self.output.clear()
            os.write(self.master, b'\x1b[15~')  # xterm F5: resume emulation.
            self.await_output(b'(Running)')

    def command(self, command):
        verb, _, argument = command.partition(' ')
        if verb == 'key' and argument:
            self.keyboard('key', '--clearmodifiers', *shlex.split(argument))
        elif verb == 'text' and argument:
            self.keyboard('type', '--clearmodifiers', '--delay', '30', '--', argument)
        elif verb == 'wait' and argument:
            seconds = float(argument)
            if not 0 <= seconds <= 60:
                raise ValueError('wait must be between 0 and 60 seconds')
            self.settle(seconds)
            return
        elif verb == 'screen' and not argument:
            return
        else:
            raise ValueError('Use screen, key KEYS, text TEXT, wait SECONDS, or quit')
        self.settle()


def interrupted(signum, _frame):
    raise KeyboardInterrupt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-c', '--command', action='append',
                        help='run a command and capture the screen; repeat for a sequence')
    parser.add_argument('--delay', type=float, default=0.2,
                        help='seconds to allow the game to react (default: 0.2)')
    args = parser.parse_args()
    if not 0 <= args.delay <= 60:
        parser.error('--delay must be between 0 and 60 seconds')
    signal.signal(signal.SIGTERM, interrupted)
    try:
        with Game(delay=args.delay) as game:
            print(game.screen(), flush=True)
            if args.command is not None:
                for command in args.command:
                    if command == 'quit':
                        break
                    game.command(command)
                    print(f'\n--- {command} ---\n{game.screen()}', flush=True)
            else:
                print('Commands: screen, key KEYS, text TEXT, wait SECONDS, quit',
                      file=sys.stderr)
                while True:
                    if sys.stdin.isatty():
                        print('dosbox> ', end='', file=sys.stderr, flush=True)
                    line = sys.stdin.readline()
                    if not line or line.rstrip('\n') == 'quit':
                        break
                    command = line.rstrip('\n')
                    if not command:
                        continue
                    try:
                        game.command(command)
                        print(game.screen(), flush=True)
                    except ValueError as error:
                        print(error, file=sys.stderr)
    except KeyboardInterrupt:
        return 130
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        print(f'dosbox_game: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
