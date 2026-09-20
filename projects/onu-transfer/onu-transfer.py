#!/usr/bin/env python3
"""Transferência verificada de arquivos para uma ONU FiberHome via Telnet + nc."""

import argparse
import hashlib
import os
import re
import secrets
import socket
import sys
import tempfile
import time
from pathlib import Path


DEFAULT_HOST = os.environ.get("ONU_HOST", "192.168.1.245")
DEFAULT_PORT = int(os.environ.get("ONU_TELNET_PORT", "23"))
HASH_RE = re.compile(r"\b([0-9a-fA-F]{64})\b")


def shell_quote(value):
    """Citação POSIX para um argumento já escolhido pelo usuário."""
    return "'" + value.replace("'", "'\"'\"'") + "'"


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def local_address_for(remote_host):
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        probe.connect((remote_host, 9))
        return probe.getsockname()[0]
    finally:
        probe.close()


class TelnetShell:
    """Shell Telnet mínimo: rejeita opções e espera marcadores únicos."""

    IAC = 255
    DO = 253
    DONT = 254
    WILL = 251
    WONT = 252
    SB = 250
    SE = 240

    def __init__(self, host, port, timeout):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.socket = None
        self.buffer = ""

    def __enter__(self):
        self.socket = socket.create_connection((self.host, self.port), self.timeout)
        self.socket.settimeout(self.timeout)
        return self

    def __exit__(self, exc_type, exc, traceback):
        if self.socket is not None:
            self.socket.close()

    def _filter_telnet(self, data):
        output = bytearray()
        index = 0
        while index < len(data):
            byte = data[index]
            if byte != self.IAC:
                output.append(byte)
                index += 1
                continue
            if index + 1 >= len(data):
                break
            command = data[index + 1]
            if command == self.IAC:
                output.append(self.IAC)
                index += 2
            elif command in (self.DO, self.DONT, self.WILL, self.WONT):
                if index + 2 >= len(data):
                    break
                option = data[index + 2]
                reply = self.WONT if command in (self.DO, self.DONT) else self.DONT
                self.socket.sendall(bytes((self.IAC, reply, option)))
                index += 3
            elif command == self.SB:
                end = data.find(bytes((self.IAC, self.SE)), index + 2)
                index = len(data) if end < 0 else end + 2
            else:
                index += 2
        return output.decode("latin1", errors="replace")

    def start(self, command):
        token = "__ONU_TRANSFER_%s__" % secrets.token_hex(12)
        full_command = "{ %s; }; rc=$?; printf '\\n%s:%%s\\n' \"$rc\"" % (command, token)
        self.socket.sendall(full_command.encode("utf-8") + b"\r\n")
        return token

    def wait(self, token):
        pattern = re.compile(re.escape(token) + r":([0-9]+)")
        deadline = time.monotonic() + self.timeout
        while True:
            match = pattern.search(self.buffer)
            if match:
                output = self.buffer[:match.start()]
                self.buffer = self.buffer[match.end():]
                return int(match.group(1)), output
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("a ONU não retornou o marcador de conclusão")
            self.socket.settimeout(remaining)
            received = self.socket.recv(4096)
            if not received:
                raise ConnectionError("a sessão Telnet foi encerrada pela ONU")
            self.buffer += self._filter_telnet(received)

    def run(self, command):
        token = self.start(command)
        return self.wait(token)


def remote_sha256(shell, remote_path):
    rc, output = shell.run("sha256sum %s" % shell_quote(remote_path))
    hashes = HASH_RE.findall(output)
    if rc != 0 or not hashes:
        raise RuntimeError("não foi possível calcular SHA-256 remoto: %s" % output.strip())
    return hashes[-1].lower()


def connect_sender(host, port, timeout):
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
        try:
            return socket.create_connection((host, port), min(1.0, timeout))
        except OSError as error:
            last_error = error
            time.sleep(0.15)
    raise TimeoutError("nc remoto não abriu a porta %d: %s" % (port, last_error))


def push(args):
    source = Path(args.local).expanduser().resolve()
    if not source.is_file():
        raise ValueError("arquivo local inexistente ou não regular: %s" % source)
    local_hash = sha256_file(source)
    transfer_port = args.transfer_port or (30000 + secrets.randbelow(20000))
    remote_temp = "%s.onu-transfer-%s.part" % (args.remote, secrets.token_hex(5))
    command = (
        "umask 077; nc -l -p %d > %s; rc=$?; "
        "if [ \"$rc\" -eq 0 ]; then mv -f %s %s || rc=$?; "
        "else rm -f %s; fi; test \"$rc\" -eq 0"
        % (transfer_port, shell_quote(remote_temp), shell_quote(remote_temp),
           shell_quote(args.remote), shell_quote(remote_temp))
    )

    with TelnetShell(args.host, args.port, args.timeout) as shell:
        token = shell.start(command)
        with connect_sender(args.host, transfer_port, args.timeout) as connection:
            with open(source, "rb") as input_file:
                for chunk in iter(lambda: input_file.read(1024 * 1024), b""):
                    connection.sendall(chunk)
        rc, output = shell.wait(token)
        if rc != 0:
            raise RuntimeError("push remoto falhou: %s" % output.strip())
        if not args.no_verify:
            remote_hash = remote_sha256(shell, args.remote)
            if remote_hash != local_hash:
                raise RuntimeError("SHA-256 divergente: local=%s remoto=%s" % (local_hash, remote_hash))
    print("push concluído: %s -> %s (%s)" % (source, args.remote, local_hash))


def pull(args):
    remote_path = args.remote
    destination = Path(args.local or Path(remote_path).name).expanduser()
    if not destination.name:
        raise ValueError("destino local inválido")
    destination.parent.mkdir(parents=True, exist_ok=True)
    advertised_host = args.local_host or local_address_for(args.host)

    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((args.bind_host, args.transfer_port or 0))
    listener.listen(1)
    listener.settimeout(args.timeout)
    transfer_port = listener.getsockname()[1]
    temp_name = None

    try:
        with TelnetShell(args.host, args.port, args.timeout) as shell:
            expected_hash = None if args.no_verify else remote_sha256(shell, remote_path)
            token = shell.start("nc %s %d < %s" % (
                shell_quote(advertised_host), transfer_port, shell_quote(remote_path)))
            try:
                connection, _ = listener.accept()
            except socket.timeout:
                rc, output = shell.wait(token)
                raise RuntimeError("a ONU não iniciou pull (rc=%d): %s" % (rc, output.strip()))
            with connection:
                descriptor, temp_name = tempfile.mkstemp(
                    prefix=".%s.onu-transfer-" % destination.name, dir=str(destination.parent))
                with os.fdopen(descriptor, "wb") as output_file:
                    while True:
                        chunk = connection.recv(1024 * 1024)
                        if not chunk:
                            break
                        output_file.write(chunk)
            rc, output = shell.wait(token)
            if rc != 0:
                raise RuntimeError("pull remoto falhou: %s" % output.strip())
        if not args.no_verify and sha256_file(temp_name) != expected_hash:
            raise RuntimeError("SHA-256 local diverge do arquivo remoto")
        os.replace(temp_name, destination)
        temp_name = None
    finally:
        listener.close()
        if temp_name and os.path.exists(temp_name):
            os.unlink(temp_name)
    print("pull concluído: %s -> %s" % (remote_path, destination))


def execute(args):
    with TelnetShell(args.host, args.port, args.timeout) as shell:
        rc, output = shell.run(args.command)
    if output:
        print(output, end="" if output.endswith("\n") else "\n")
    if rc:
        raise RuntimeError("comando remoto retornou %d" % rc)


def parse_args():
    parser = argparse.ArgumentParser(
        description="push/pull verificado para ONU via Telnet root + BusyBox nc")
    parser.add_argument("--host", default=DEFAULT_HOST, help="IP da ONU (padrão: %(default)s)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help="porta Telnet (padrão: %(default)s)")
    parser.add_argument("--timeout", type=float, default=20,
                        help="timeout por operação em segundos (padrão: %(default)s)")
    parser.add_argument("--transfer-port", type=int,
                        help="porta TCP temporária; aleatória por padrão")
    parser.add_argument("--no-verify", action="store_true",
                        help="não comparar SHA-256 após a transferência")
    subcommands = parser.add_subparsers(dest="operation", required=True)

    push_parser = subcommands.add_parser("push", help="PC -> ONU")
    push_parser.add_argument("local")
    push_parser.add_argument("remote")
    push_parser.set_defaults(handler=push)

    pull_parser = subcommands.add_parser("pull", help="ONU -> PC")
    pull_parser.add_argument("remote")
    pull_parser.add_argument("local", nargs="?")
    pull_parser.add_argument("--local-host", help="IP do PC visível pela ONU")
    pull_parser.add_argument("--bind-host", default="0.0.0.0",
                             help="endereço local de escuta (padrão: %(default)s)")
    pull_parser.set_defaults(handler=pull)

    exec_parser = subcommands.add_parser("exec", help="executa comando remoto")
    exec_parser.add_argument("command")
    exec_parser.set_defaults(handler=execute)
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        args.handler(args)
    except (OSError, ValueError, RuntimeError, TimeoutError, ConnectionError) as error:
        print("erro: %s" % error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
