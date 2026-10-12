"""QEMU virtual machine on a serial console, driven with pexpect."""
import os
import re
import shutil
import time

import pexpect

PROMPT = "E2E-PROMPT# "


class VM:
    def __init__(self, name, args, log_dir, memory=1536, cpus=2):
        self.name = name
        self.log_path = os.path.join(log_dir, f"{name}.log")
        # Guest reboots stay in the same QEMU process: several scenarios span
        # more than one boot. The i6300esb watchdog resets a hung guest.
        cmd = ["qemu-system-x86_64", "-name", name, "-m", str(memory), "-smp", str(cpus), "-nographic",
               "-serial", "mon:stdio",
               "-netdev", "user,id=n0", "-device", "virtio-net-pci,netdev=n0",
               "-device", "i6300esb", "-action", "watchdog=reset"] + args
        if os.access("/dev/kvm", os.R_OK | os.W_OK):
            cmd += ["-accel", "kvm"]
        self.log = open(self.log_path, "a", encoding="utf-8", errors="replace")
        self.log.write(f"\n===== {time.strftime('%H:%M:%S')} {' '.join(cmd)}\n")
        self.child = pexpect.spawn(cmd[0], cmd[1:], encoding="utf-8", codec_errors="replace",
                                   timeout=60, maxread=65536, dimensions=(50, 250))
        self.child.logfile_read = self.log

    def expect(self, patterns, timeout=300):
        return self.child.expect(patterns, timeout=timeout)

    def wait_shell(self, timeout=600):
        """Wait for the root autologin on the serial console and set a fixed prompt."""
        self.child.expect([r"root@[^:\s]*:[^#\s]*# "], timeout=timeout)
        self.child.sendline(f"export PS1='{PROMPT}'; stty cols 250; export SYSTEMD_PAGER= TERM=dumb")
        self.child.expect_exact(PROMPT, timeout=30)
        self.child.expect_exact(PROMPT, timeout=30)

    def run(self, command, timeout=120, check=True):
        """Run a shell command; returns (exit code, output)."""
        marker = f"__E2E_RC_{int(time.time() * 1000)}__"
        self.child.sendline(f"{command}; echo {marker}=$?")
        self.child.expect(rf"{marker}=(\d+)", timeout=timeout)
        rc = int(self.child.match.group(1))
        out = self.child.before
        # Drop the echoed command line.
        out = out.split("\n", 1)[1] if "\n" in out else ""
        out = out.replace("\r", "")
        self.child.expect_exact(PROMPT, timeout=30)
        if check and rc != 0:
            raise AssertionError(f"[{self.name}] command failed ({rc}): {command}\n{out}")
        return rc, out

    def out(self, command, timeout=120):
        return self.run(command, timeout=timeout)[1].strip()

    def wait_exit(self, timeout=300):
        self.child.expect(pexpect.EOF, timeout=timeout)

    def kill(self):
        if self.child.isalive():
            self.child.terminate(force=True)
        self.log.close()


def ovmf_vars(dest):
    """A private copy of OVMF's variable store for a UEFI machine."""
    for src in ("/usr/share/OVMF/OVMF_VARS_4M.fd", "/usr/share/OVMF/OVMF_VARS.fd"):
        if os.path.exists(src):
            shutil.copy(src, dest)
            return
    raise RuntimeError("OVMF not installed")


def ovmf_code():
    for src in ("/usr/share/OVMF/OVMF_CODE_4M.fd", "/usr/share/OVMF/OVMF_CODE.fd"):
        if os.path.exists(src):
            return src
    raise RuntimeError("OVMF not installed")


def search(pattern, text):
    m = re.search(pattern, text, re.M)
    return m.group(1) if m else None
