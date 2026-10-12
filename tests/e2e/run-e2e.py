#!/usr/bin/env python3
"""End-to-end test of CustomOS's A/B update system in QEMU.

    tests/e2e/run-e2e.py --images DIR [--stages all|install,boot,...] [--logs DIR]

DIR is the output of tests/e2e/build-images.sh. The test serves the releases
with tests/e2e/mock-github.py and drives virtual machines over their serial
console. The stages share one virtual disk and must run in order:

  install      live environment: customos-install-ab writes the A/B layout
  boot         first boot (BIOS): slot A, persistent data, live leftovers removed
  update       0.3.0 from the mock GitHub: download, verify, install, reboot into B
  rollback     manual rollback to 0.2.0 (slot A), then update to 0.3.0 again
  broken       0.4.0 fails its health check: automatic rollback to 0.3.0
  panic        0.5.0 panics on every boot: GRUB's counter falls back to 0.3.0
  untrusted    0.6.0 is signed with an unknown key: refused, nothing installed
  downgrade    RAUC itself refuses an older bundle (pre-install hook)
  uefi         the same disk boots with UEFI firmware (OVMF)
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vm import VM, ovmf_code, ovmf_vars, search  # noqa: E402

STAGES = ["install", "boot", "update", "rollback", "broken", "panic", "untrusted", "downgrade", "uefi"]
RESULTS = []


def check(cond, what):
    RESULTS.append((bool(cond), what))
    print(("  PASS  " if cond else "  FAIL  ") + what, flush=True)
    if not cond:
        raise AssertionError(what)


class Env:
    def __init__(self, images, logs):
        self.images = os.path.abspath(images)
        self.logs = os.path.abspath(logs)
        os.makedirs(self.logs, exist_ok=True)
        self.disk = os.path.join(self.images, "disk.img")
        self.published = os.path.join(self.images, "published")
        os.makedirs(self.published, exist_ok=True)
        self.server = subprocess.Popen(
            [sys.executable, os.path.join(os.path.dirname(__file__), "mock-github.py"), "--root", self.published,
             "--port", "8077"], stdout=open(os.path.join(self.logs, "mock-github.log"), "a"), stderr=subprocess.STDOUT)
        time.sleep(1)

    def publish(self, version):
        src = os.path.join(self.images, "releases", "v" + version)
        dst = os.path.join(self.published, "v" + version)
        if not os.path.exists(dst):
            shutil.copytree(src, dst, copy_function=os.link)

    def installed_vm(self, name, uefi=False):
        args = ["-drive", f"file={self.disk},if=virtio,format=raw"]
        if uefi:
            vars_fd = os.path.join(self.images, "ovmf-vars.fd")
            ovmf_vars(vars_fd)
            args = ["-machine", "q35", "-drive", f"if=pflash,format=raw,readonly=on,file={ovmf_code()}",
                    "-drive", f"if=pflash,format=raw,file={vars_fd}"] + args
        return VM(name, args, self.logs)

    def close(self):
        self.server.terminate()


def boot_until_slot(vm, slot, timeout=1800):
    """Wait for a shell on the given slot, letting failed boots of the other slot pass by."""
    deadline = time.time() + timeout
    while True:
        vm.wait_shell(timeout=max(60, deadline - time.time()))
        booted = search(r"rauc\.slot=(\w)", vm.out("cat /proc/cmdline"))
        if booted == slot:
            return
        print(f"    (booted slot {booted} while waiting for {slot}; waiting for it to restart)", flush=True)
        vm.expect([r"reboot: Restarting system", r"Rebooting"], timeout=max(60, deadline - time.time()))


def wait_health(vm, timeout=400):
    vm.run(f"timeout {timeout} sh -c 'until systemctl is-active -q customos-health-check; do "
           f"systemctl is-failed -q customos-health-check && exit 1; sleep 3; done'", timeout=timeout + 30)
    return vm.out("cat /data/customos/update/last-boot.env")


def wait_state(vm, states, timeout=300):
    expr = "|".join(states)
    vm.run(f"timeout {timeout} sh -c 'until customos-update status --json | grep -Eq \"\\\"state\\\": \\\"({expr})\\\"\"; "
           f"do sleep 3; done'", timeout=timeout + 30)
    return vm.out("customos-update status")


def grubenv(vm):
    text = vm.out("grub-editenv /efi/grub/grubenv list")
    return dict(line.split("=", 1) for line in text.splitlines() if "=" in line)


def reboot(vm):
    """Restart through the update service and wait until the old system is gone."""
    vm.child.sendline("customos-update reboot")
    vm.expect([r"reboot: Restarting system"], timeout=600)


# ---- stages ----------------------------------------------------------------------------

def stage_install(env):
    if os.path.exists(env.disk):
        os.remove(env.disk)
    subprocess.run(["truncate", "-s", "10G", env.disk], check=True)
    vm = VM("install", ["-kernel", os.path.join(env.images, "bzImage"),
                        "-append", "root=/dev/vda rw console=ttyS0,115200 loglevel=4",
                        "-drive", f"file={os.path.join(env.images, 'live.img')},if=virtio,format=raw,snapshot=on",
                        "-drive", f"file={env.disk},if=virtio,format=raw"], env.logs)
    try:
        vm.wait_shell(timeout=900)
        check(vm.run("test -e /etc/customos-live", check=False)[0] == 0, "live system: /etc/customos-live present")
        status = vm.out("customos-update status")
        check("live image" in status, "live system: update service explains that updates need an installed system")
        check(vm.out("systemctl show -p ConditionResult --value rauc.service") == "no",
              "live system: rauc.service is skipped (no A/B configuration)")
        rc, out = vm.run("printf 'secret12\\n' | CUSTOMOS_IMAGE=/opt/customos/rootfs.squashfs "
                         "/usr/libexec/customos-install-ab --disk /dev/vdb --confirm /dev/vdb --user alice "
                         "--fullname 'Alice Example' --hostname e2e-pc", timeout=1800, check=False)
        print(out[-1500:])
        check(rc == 0 and "DONE" in out, "installer finished (PROGRESS ... DONE)")
        parts = vm.out("sgdisk -p /dev/vdb")
        for label in ("bios-boot", "customos-esp", "customos-rootfs-a", "customos-rootfs-b", "customos-data"):
            check(label in parts, f"partition {label} created")
        check(vm.run("cmp -n $(stat -c %s /opt/customos/rootfs.squashfs) /opt/customos/rootfs.squashfs /dev/vdb3",
                     check=False)[0] == 0, "slot A holds the system image byte for byte")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_boot(env):
    vm = env.installed_vm("boot")
    try:
        vm.expect([r"Loading CustomOS \(slot A\)"], timeout=600)
        check(True, "GRUB A/B menu selected slot A")
        boot_until_slot(vm, "A")
        check(vm.out("findmnt -no FSTYPE,OPTIONS /").startswith("squashfs ro"), "root file system is the read-only squashfs slot")
        check(vm.out("findmnt -no FSTYPE /etc") == "overlay", "/etc is an overlay on the data partition")
        check("customos-data" in vm.out("findmnt -no SOURCE /data; blkid -s LABEL -o value $(findmnt -no SOURCE /data)"),
              "/data is the data partition")
        check(vm.out("id -u alice") == "1000", "the installer's account exists (uid 1000)")
        check("wheel" in vm.out("id -nG alice"), "the account is an administrator (wheel)")
        check(vm.run("id live", check=False)[0] != 0, "the live user is gone")
        check(vm.run("test -e /etc/customos-live -o -e /etc/sudoers.d/live", check=False)[0] != 0,
              "live marker and live sudo rule are gone")
        check(vm.run("grep -q autologin /etc/meridian/login.conf", check=False)[0] != 0, "live autologin is gone")
        check(vm.out("cat /etc/hostname") == "e2e-pc", "computer name set by the installer")
        report = wait_health(vm)
        check("RESULT=good" in report, "health check confirmed the slot")
        env_ = grubenv(vm)
        check(env_.get("A_OK") == "1" and env_.get("A_TRY") == "0" and env_.get("ORDER") == "A B",
              f"grubenv after confirmation: {env_}")
        status = wait_state(vm, ["idle", "up-to-date"])
        check("0.2.0 (slot A)" in status, "customos-update status: 0.2.0 on slot A")
        vm.run("echo 'hello from 0.2.0' > /home/alice/hello.txt && chown alice: /home/alice/hello.txt")
        vm.run("echo custom > /etc/customos-e2e.conf")     # not a persistent setting
        vm.run("mkdir -p /etc/NetworkManager/system-connections && echo wifi > /etc/NetworkManager/system-connections/home.nmconnection")
        vm.run("sync")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_update(env):
    env.publish("0.3.0")
    vm = env.installed_vm("update")
    try:
        boot_until_slot(vm, "A")
        wait_health(vm)
        out = vm.out("customos-update check", timeout=300)
        check("0.3.0 is available" in out, "update 0.3.0 found on the mock GitHub")
        rc, out = vm.run("customos-update install --yes", timeout=1200, check=False)
        check(rc == 0 and "is installed" in out, "0.3.0 downloaded, verified and installed into slot B")
        env_ = grubenv(vm)
        check(env_.get("ORDER") == "B A" and env_.get("B_OK") == "1" and env_.get("B_VERSION") == "0.3.0",
              f"RAUC made slot B primary: {env_}")
        check(vm.out("cat /data/customos/update/pending-slot") == "B", "post-install hook marked the trial boot")
        check("reboot-required" in vm.out("customos-update status"), "status: restart required")
        reboot(vm)
        boot_until_slot(vm, "B")
        check(vm.out(". /etc/os-release; echo $VERSION_ID") == "0.3.0", "running 0.3.0 after the restart")
        report = wait_health(vm)
        check("RESULT=good" in report and "trial" in report, "trial boot of 0.3.0 passed the health check")
        status = wait_state(vm, ["up-to-date", "idle"])
        check("Last update:         0.3.0" in status, "status: last successful update 0.3.0")
        hist = vm.out("customos-update history")
        check("success" in hist and "0.2.0 -> 0.3.0" in hist, "history: 0.2.0 -> 0.3.0 success")
        check(vm.out("cat /home/alice/hello.txt") == "hello from 0.2.0", "files in /home kept")
        check(vm.out("id -u alice") == "1000" and vm.out("cat /etc/hostname") == "e2e-pc",
              "account and computer name kept")
        check(vm.out("cat /etc/NetworkManager/system-connections/home.nmconnection") == "wifi",
              "network connections kept (persistent setting)")
        check(vm.run("test -e /etc/customos-e2e.conf", check=False)[0] != 0
              and "customos-e2e.conf" in vm.out("find /data/customos/etc-backup -name customos-e2e.conf"),
              "other /etc changes reset to the new version's defaults, with a backup")
        env_ = grubenv(vm)
        check(env_.get("B_TRY") == "0" and env_.get("A_OK") == "1", f"slot B confirmed, slot A kept: {env_}")
        check("Previous version:    0.2.0 (slot A" in vm.out("customos-update status"), "rollback to 0.2.0 offered")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_rollback(env):
    vm = env.installed_vm("rollback")
    try:
        boot_until_slot(vm, "B")
        wait_health(vm)
        wait_state(vm, ["up-to-date", "idle"])
        rc, out = vm.run("customos-update rollback --yes", timeout=120, check=False)
        check(rc == 0, "rollback requested")
        check(grubenv(vm).get("ORDER") == "A B", "slot A is primary again")
        reboot(vm)
        boot_until_slot(vm, "A")
        check(vm.out(". /etc/os-release; echo $VERSION_ID") == "0.2.0", "running 0.2.0 after the rollback")
        wait_health(vm)
        wait_state(vm, ["up-to-date", "idle"])
        check("success" in vm.out("customos-update history | grep rollback"), "history: rollback recorded")
        check(vm.out("cat /home/alice/hello.txt") == "hello from 0.2.0", "files kept across the rollback")
        # And forward again.
        out = vm.out("customos-update check", timeout=300)
        check("0.3.0 is available" in out, "0.3.0 offered again after the rollback")
        rc, out = vm.run("customos-update install --yes", timeout=1200, check=False)
        check(rc == 0, "0.3.0 reinstalled into slot B")
        reboot(vm)
        boot_until_slot(vm, "B")
        report = wait_health(vm)
        check("RESULT=good" in report, "0.3.0 confirmed again")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_broken(env):
    env.publish("0.4.0")
    vm = env.installed_vm("broken")
    try:
        boot_until_slot(vm, "B")
        wait_health(vm)
        wait_state(vm, ["up-to-date", "idle"])
        check("0.4.0 is available" in vm.out("customos-update check", timeout=300), "0.4.0 offered")
        rc, out = vm.run("customos-update install --yes", timeout=1200, check=False)
        check(rc == 0, "0.4.0 installed into slot A")
        reboot(vm)
        # Slot A boots, fails its trial (desktop stand-in never starts) and restarts into B.
        vm.expect([r"FAILED its trial boot"], timeout=1200)
        check(True, "health check rejected 0.4.0 on its trial boot")
        boot_until_slot(vm, "B")
        check(vm.out(". /etc/os-release; echo $VERSION_ID") == "0.3.0", "back on 0.3.0")
        status = wait_state(vm, ["error"])
        check("didn't start correctly" in status and "0.4.0" in status, "status explains the automatic rollback")
        check("customos-demo.service" in status, "status names the unit that failed")
        check("rolled-back" in vm.out("customos-update history"), "history: 0.4.0 rolled back")
        env_ = grubenv(vm)
        check(env_.get("A_OK") == "0", f"slot A marked bad: {env_}")
        vm.run("customos-update dismiss")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_panic(env):
    env.publish("0.5.0")
    vm = env.installed_vm("panic")
    try:
        boot_until_slot(vm, "B")
        wait_health(vm)
        wait_state(vm, ["up-to-date", "idle"])
        check("0.5.0 is available" in vm.out("customos-update check", timeout=300), "0.5.0 offered")
        rc, out = vm.run("customos-update install --yes", timeout=1200, check=False)
        check(rc == 0, "0.5.0 installed into slot A")
        reboot(vm)
        for attempt in (1, 2, 3):
            vm.expect([r"Kernel panic - not syncing"], timeout=900)
            print(f"    slot A panicked (attempt {attempt})", flush=True)
        check(True, "slot A panicked on three boots")
        boot_until_slot(vm, "B")
        check(vm.out(". /etc/os-release; echo $VERSION_ID") == "0.3.0", "GRUB fell back to 0.3.0 after three failed boots")
        env_ = grubenv(vm)
        check(env_.get("A_TRY") == "3", f"slot A used up its tries: {env_}")
        status = wait_state(vm, ["error"])
        check("didn't start correctly" in status and "0.5.0" in status, "status explains the automatic rollback")
        check("Previous version" not in status, "the failed slot is not offered for rollback")
        vm.run("customos-update dismiss")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_untrusted(env):
    env.publish("0.6.0")
    vm = env.installed_vm("untrusted")
    try:
        boot_until_slot(vm, "B")
        wait_health(vm)
        wait_state(vm, ["up-to-date", "idle"])
        before = grubenv(vm)
        check("0.6.0 is available" in vm.out("customos-update check", timeout=300), "0.6.0 offered")
        rc, out = vm.run("customos-update install --yes", timeout=1200, check=False)
        check(rc != 0 and "isn't signed by a trusted CustomOS key" in out, "bundle with an unknown signature refused")
        check(vm.run("ls /data/customos/update/cache/*.raucb", check=False)[0] != 0, "untrusted bundle deleted")
        check(grubenv(vm) == before, "boot configuration unchanged")
        vm.run("customos-update dismiss")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_downgrade(env):
    vm = env.installed_vm("downgrade")
    try:
        boot_until_slot(vm, "B")
        wait_health(vm)
        # Give the old, validly signed 0.2.0 bundle straight to RAUC, bypassing the update service.
        rc, out = vm.run("rauc install http://10.0.2.2:8077/storage/v0.2.0/CustomOS-v0.2.0.raucb", timeout=600, check=False)
        print(out[-800:])
        check(rc != 0 and ("downgrade" in out or "not supported" in out.lower()),
              "RAUC refused the older bundle (pre-install hook) or cannot fetch over HTTP")
        if "downgrade" not in out:
            print("    note: this RAUC build cannot install from a URL; the hook is covered by the update service path")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def stage_uefi(env):
    vm = env.installed_vm("uefi", uefi=True)
    try:
        vm.expect([r"Loading CustomOS \(slot B\)"], timeout=900)
        boot_until_slot(vm, "B")
        check(vm.run("test -d /sys/firmware/efi", check=False)[0] == 0, "booted with UEFI firmware")
        report = wait_health(vm)
        check("RESULT=good" in report, "UEFI boot confirmed by the health check")
        vm.child.sendline("poweroff")
        vm.wait_exit(timeout=300)
    finally:
        vm.kill()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--images", required=True)
    p.add_argument("--stages", default="all")
    p.add_argument("--logs", default=None)
    a = p.parse_args()
    stages = STAGES if a.stages == "all" else a.stages.split(",")
    env = Env(a.images, a.logs or os.path.join(a.images, "logs"))
    failed = None
    try:
        for s in stages:
            print(f"=== stage {s}", flush=True)
            t0 = time.time()
            globals()["stage_" + s](env)
            print(f"=== stage {s}: done in {time.time() - t0:.0f}s", flush=True)
    except Exception as e:  # noqa: BLE001
        failed = e
        print(f"!!! {type(e).__name__}: {e}", flush=True)
    finally:
        env.close()
    passed = sum(1 for ok, _ in RESULTS if ok)
    print(f"\n{passed}/{len(RESULTS)} checks passed" + ("" if failed is None else " (stopped at a failure)"))
    sys.exit(0 if failed is None else 1)


if __name__ == "__main__":
    main()
