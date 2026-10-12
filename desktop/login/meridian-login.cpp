// meridian-login: the sign-in service for the Meridian desktop.
//
// Runs as root on tty1 (systemd: meridian-login.service). In a loop it:
//   1. on the first pass, if /etc/meridian/login.conf names an `autologin`
//      user, starts that user's session directly (used by the live image);
//   2. otherwise starts the greeter (meridian-greeter, a full-screen Qt UI)
//      and talks to it over pipes:
//         greeter -> us:  "LOGIN\t<user>\t<password>\n" | "POWEROFF\n" | "REBOOT\n"
//         us -> greeter:  "OK\n" | "FAIL\t<message>\n"
//   3. authenticates with PAM (service "meridian"), opens a PAM session
//      (pam_systemd registers it with logind on seat0/VT1), drops privileges
//      and runs `meridian-session` as the user;
//   4. when the session ends, closes the PAM session and goes back to 2.
//
// Each user session runs in a forked "session leader" process that owns the
// PAM handle, exactly like login(1) does.

#include <security/pam_appl.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifndef MERIDIAN_LIBEXECDIR
#define MERIDIAN_LIBEXECDIR "/usr/libexec"
#endif

namespace {

const char *kTty = "tty1";
const char *kTtyPath = "/dev/tty1";
const char *kVt = "1";

void logmsg(const char *fmt, const std::string &a = std::string())
{
    std::fprintf(stderr, "meridian-login: ");
    std::fprintf(stderr, fmt, a.c_str());
    std::fprintf(stderr, "\n");
}

std::string readConfigValue(const char *key)
{
    FILE *f = std::fopen("/etc/meridian/login.conf", "r");
    if (!f)
        return {};
    char line[512];
    std::string result;
    const size_t klen = std::strlen(key);
    while (std::fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '#' || *p == '[') continue;
        if (std::strncmp(p, key, klen) == 0) {
            p += klen;
            while (*p == ' ' || *p == '\t') ++p;
            if (*p != '=') continue;
            ++p;
            while (*p == ' ' || *p == '\t') ++p;
            result = p;
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
                result.pop_back();
        }
    }
    std::fclose(f);
    return result;
}

// ---- PAM conversation ------------------------------------------------------

struct Conv {
    std::string password;
    std::string lastMessage;
};

int conversation(int n, const struct pam_message **msg, struct pam_response **resp, void *data)
{
    auto *c = static_cast<Conv *>(data);
    auto *r = static_cast<pam_response *>(std::calloc(size_t(n), sizeof(pam_response)));
    if (!r)
        return PAM_BUF_ERR;
    for (int i = 0; i < n; ++i) {
        switch (msg[i]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
        case PAM_PROMPT_ECHO_ON:
            r[i].resp = strdup(c->password.c_str());
            break;
        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            if (msg[i]->msg)
                c->lastMessage = msg[i]->msg;
            break;
        }
    }
    *resp = r;
    return PAM_SUCCESS;
}

void putenvPam(pam_handle_t *h, const std::string &kv)
{
    pam_putenv(h, kv.c_str());
}

// ---- the session ---------------------------------------------------------

// Runs in the forked session-leader process. Authenticates (unless
// autologin), reports the result through `reportFd`, then runs the session.
// Returns the process exit code.
int sessionLeader(const std::string &user, const std::string &password, bool autologin, int reportFd, int goFd)
{
    auto report = [reportFd](const std::string &s) {
        if (reportFd >= 0) {
            const std::string line = s + "\n";
            (void)!write(reportFd, line.data(), line.size());
        }
    };

    Conv conv{password, {}};
    pam_conv pc{conversation, &conv};
    pam_handle_t *h = nullptr;
    const char *service = autologin ? "meridian-autologin" : "meridian";
    int rc = pam_start(service, user.c_str(), &pc, &h);
    if (rc != PAM_SUCCESS) {
        report(std::string("FAIL\tPAM is not configured (") + pam_strerror(h, rc) + ")");
        return 1;
    }
    pam_set_item(h, PAM_TTY, kTty);
    pam_set_item(h, PAM_RHOST, "");

    if (!autologin) {
        rc = pam_authenticate(h, 0);
        if (rc != PAM_SUCCESS) {
            pam_end(h, rc);
            sleep(1);   // slow down guessing
            report(rc == PAM_AUTH_ERR || rc == PAM_USER_UNKNOWN || rc == PAM_MAXTRIES
                       ? "FAIL\tThe password is incorrect."
                       : std::string("FAIL\t") + pam_strerror(h, rc));
            return 1;
        }
    }
    rc = pam_acct_mgmt(h, 0);
    if (rc == PAM_NEW_AUTHTOK_REQD) {
        pam_end(h, rc);
        report("FAIL\tYour password has expired. Sign in on a text console (Ctrl+Alt+F2) to change it.");
        return 1;
    }
    if (rc != PAM_SUCCESS) {
        pam_end(h, rc);
        report(std::string("FAIL\t") + (conv.lastMessage.empty() ? "This account cannot sign in." : conv.lastMessage));
        return 1;
    }

    struct passwd *pw = getpwnam(user.c_str());
    if (!pw) {
        pam_end(h, PAM_USER_UNKNOWN);
        report("FAIL\tUnknown user.");
        return 1;
    }
    const uid_t uid = pw->pw_uid;
    const gid_t gid = pw->pw_gid;
    const std::string home = pw->pw_dir ? pw->pw_dir : "/";
    const std::string shell = (pw->pw_shell && *pw->pw_shell) ? pw->pw_shell : "/bin/sh";

    // Tell pam_systemd what kind of session this is.
    putenvPam(h, "XDG_SEAT=seat0");
    putenvPam(h, std::string("XDG_VTNR=") + kVt);
    putenvPam(h, "XDG_SESSION_TYPE=wayland");
    putenvPam(h, "XDG_SESSION_CLASS=user");
    putenvPam(h, "XDG_SESSION_DESKTOP=meridian");

    if (initgroups(user.c_str(), gid) != 0) {
        pam_end(h, PAM_SYSTEM_ERR);
        report("FAIL\tCould not set up the user's groups.");
        return 1;
    }
    rc = pam_setcred(h, PAM_ESTABLISH_CRED);
    if (rc != PAM_SUCCESS) {
        pam_end(h, rc);
        report(std::string("FAIL\t") + pam_strerror(h, rc));
        return 1;
    }
    rc = pam_open_session(h, 0);
    if (rc != PAM_SUCCESS) {
        pam_setcred(h, PAM_DELETE_CRED);
        pam_end(h, rc);
        report(std::string("FAIL\tCould not start the session: ") + pam_strerror(h, rc));
        return 1;
    }

    // Authentication succeeded and the session is open.
    report("OK");
    if (reportFd >= 0)
        close(reportFd);
    // Wait until the daemon says the screen is free (the greeter has exited).
    if (goFd >= 0) {
        char go;
        while (read(goFd, &go, 1) < 0 && errno == EINTR) {}
        close(goFd);
    }

    // The terminal belongs to the user while signed in (as login(1) does).
    (void)!chown(kTtyPath, uid, gid);
    chmod(kTtyPath, 0620);

    std::vector<std::string> env;
    if (char **pamEnv = pam_getenvlist(h)) {
        for (char **e = pamEnv; *e; ++e) {
            env.emplace_back(*e);
            std::free(*e);
        }
        std::free(pamEnv);
    }

    const pid_t child = fork();
    if (child < 0) {
        pam_close_session(h, 0);
        pam_setcred(h, PAM_DELETE_CRED);
        pam_end(h, PAM_SYSTEM_ERR);
        return 1;
    }
    if (child == 0) {
        // ---- the user's process ----
        if (setgid(gid) != 0 || setuid(uid) != 0) {
            logmsg("cannot drop privileges for %s", user);
            _exit(1);
        }
        if (setuid(0) == 0) _exit(1);   // paranoia: must not regain root
        if (chdir(home.c_str()) != 0)
            (void)!chdir("/");

        clearenv();
        for (const std::string &kv : env)
            putenv(strdup(kv.c_str()));
        setenv("HOME", home.c_str(), 1);
        setenv("USER", user.c_str(), 1);
        setenv("LOGNAME", user.c_str(), 1);
        setenv("SHELL", shell.c_str(), 1);
        setenv("PATH", "/usr/local/bin:/usr/bin:/bin", 1);
        if (!getenv("LANG")) setenv("LANG", "C.UTF-8", 1);
        setenv("XDG_SEAT", "seat0", 1);
        setenv("XDG_VTNR", kVt, 1);
        setenv("XDG_SESSION_TYPE", "wayland", 1);
        setenv("XDG_SESSION_CLASS", "user", 1);
        setenv("XDG_SESSION_DESKTOP", "meridian", 1);
        if (!getenv("XDG_RUNTIME_DIR")) {
            const std::string run = "/run/user/" + std::to_string(uid);
            setenv("XDG_RUNTIME_DIR", run.c_str(), 1);
        }

        // A login shell reads /etc/profile and ~/.profile, then starts the desktop.
        execl(shell.c_str(), shell.c_str(), "-l", "-c", "exec meridian-session", static_cast<char *>(nullptr));
        execl("/bin/sh", "sh", "-l", "-c", "exec meridian-session", static_cast<char *>(nullptr));
        _exit(127);
    }

    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}

    (void)!chown(kTtyPath, 0, 0);
    chmod(kTtyPath, 0600);
    pam_close_session(h, 0);
    pam_setcred(h, PAM_DELETE_CRED);
    pam_end(h, PAM_SUCCESS);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

// Forks a session leader. Returns its pid and fills `result` with "OK" or
// "FAIL\t<message>". After "OK" the caller must write one byte to `*goFd`
// (when the screen is free) and then wait for the leader to exit.
pid_t startLeader(const std::string &user, const std::string &password, bool autologin,
                  std::string *result, int *goFd)
{
    int rep[2], go[2];
    if (pipe2(rep, O_CLOEXEC) != 0)
        return -1;
    if (pipe2(go, O_CLOEXEC) != 0) {
        close(rep[0]); close(rep[1]);
        return -1;
    }
    const pid_t leader = fork();
    if (leader < 0) {
        close(rep[0]); close(rep[1]); close(go[0]); close(go[1]);
        return -1;
    }
    if (leader == 0) {
        close(rep[0]);
        close(go[1]);
        signal(SIGCHLD, SIG_DFL);
        _exit(sessionLeader(user, password, autologin, rep[1], go[0]));
    }
    close(rep[1]);
    close(go[0]);
    std::string line;
    char ch;
    while (read(rep[0], &ch, 1) == 1 && ch != '\n')
        line += ch;
    close(rep[0]);
    *result = line.empty() ? std::string("FAIL\tSign-in failed.") : line;
    *goFd = go[1];
    if (*result != "OK") {
        close(go[1]);
        *goFd = -1;
        int st;
        waitpid(leader, &st, 0);
    }
    return leader;
}

void releaseAndWait(pid_t leader, int goFd)
{
    if (goFd >= 0) {
        (void)!write(goFd, "g", 1);
        close(goFd);
    }
    int st;
    while (waitpid(leader, &st, 0) < 0 && errno == EINTR) {}
}

// ---- greeter -----------------------------------------------------------------

struct Greeter {
    pid_t pid = -1;
    FILE *in = nullptr;   // we read the greeter's requests
    int out = -1;         // we write replies
};

Greeter startGreeter()
{
    Greeter g;
    int toUs[2], toGreeter[2];
    // Close-on-exec so these never leak into a user session.
    if (pipe2(toUs, O_CLOEXEC) != 0 || pipe2(toGreeter, O_CLOEXEC) != 0)
        return g;
    g.pid = fork();
    if (g.pid == 0) {
        dup2(toGreeter[0], STDIN_FILENO);
        dup2(toUs[1], STDOUT_FILENO);
        close(toUs[0]); close(toUs[1]); close(toGreeter[0]); close(toGreeter[1]);
        const char *greeter = MERIDIAN_LIBEXECDIR "/meridian-greeter";
        execl(greeter, greeter, static_cast<char *>(nullptr));
        execlp("meridian-greeter", "meridian-greeter", static_cast<char *>(nullptr));
        _exit(127);
    }
    close(toUs[1]);
    close(toGreeter[0]);
    g.in = fdopen(toUs[0], "r");
    g.out = toGreeter[1];
    return g;
}

void stopGreeter(Greeter &g)
{
    if (g.out >= 0) close(g.out);
    if (g.in) std::fclose(g.in);
    if (g.pid > 0) {
        int st;
        // Give it a moment to exit by itself (it quits after "OK").
        for (int i = 0; i < 30; ++i) {
            if (waitpid(g.pid, &st, WNOHANG) == g.pid) { g.pid = -1; break; }
            usleep(100 * 1000);
        }
        if (g.pid > 0) {
            kill(g.pid, SIGTERM);
            waitpid(g.pid, &st, 0);
        }
    }
    g = Greeter();
}

void sendLine(int fd, const std::string &s)
{
    const std::string line = s + "\n";
    (void)!write(fd, line.data(), line.size());
}

} // namespace

int main()
{
    if (geteuid() != 0) {
        std::fprintf(stderr, "meridian-login must run as root (it is started by meridian-login.service)\n");
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);

    const std::string autologin = readConfigValue("autologin");
    bool firstPass = true;

    while (true) {
        if (firstPass && !autologin.empty() && getpwnam(autologin.c_str())) {
            firstPass = false;
            std::string reply;
            int go = -1;
            const pid_t leader = startLeader(autologin, std::string(), true, &reply, &go);
            if (leader > 0 && reply == "OK")
                releaseAndWait(leader, go);
            else
                logmsg("automatic sign-in failed: %s", reply);
            continue;
        }
        firstPass = false;

        Greeter g = startGreeter();
        if (g.pid < 0 || !g.in) {
            logmsg("cannot start the greeter%s", "");
            sleep(3);
            continue;
        }

        bool started = false;
        char buf[4096];
        while (!started && std::fgets(buf, sizeof buf, g.in)) {
            std::string line(buf);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
            if (line == "POWEROFF" || line == "REBOOT") {
                stopGreeter(g);
                execl("/bin/systemctl", "systemctl", line == "POWEROFF" ? "poweroff" : "reboot", static_cast<char *>(nullptr));
                execlp("systemctl", "systemctl", line == "POWEROFF" ? "poweroff" : "reboot", static_cast<char *>(nullptr));
                return 1;
            }
            if (line.rfind("LOGIN\t", 0) != 0)
                continue;
            const size_t t1 = line.find('\t', 6);
            if (t1 == std::string::npos) continue;
            const std::string user = line.substr(6, t1 - 6);
            std::string password = line.substr(t1 + 1);
            std::memset(buf, 0, sizeof buf);

            // Authenticate and open the session; the user's desktop only
            // starts once the greeter has quit and released the screen.
            std::string result;
            int go = -1;
            const pid_t leader = startLeader(user, password, false, &result, &go);
            password.assign(password.size(), '\0');
            if (leader > 0 && result == "OK") {
                sendLine(g.out, "OK");
                stopGreeter(g);
                started = true;
                releaseAndWait(leader, go);
            } else {
                sendLine(g.out, result);
            }
        }
        if (!started) {
            // Greeter exited or crashed without signing in: start it again.
            stopGreeter(g);
            sleep(1);
        }
    }
}
