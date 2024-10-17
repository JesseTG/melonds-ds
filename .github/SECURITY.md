# Security Policy

melonDS DS (including the underlying emulator)
is intended for recreational purposes
and should not be used in security-critical environments.

Only the latest release is supported;
_security fixes will not be backported to older releases_.

melonDS DS is only intended to execute code for the hardware it emulates;
any bug that allows it to execute arbitrary code on the host
is a vulnerability and should be reported.

If you discover such a bug, please submit a private vulnerability report
(**not** a public bug)
with a homebrew ROM that demonstrates the issue.

I will share this information with the maintainers of upstream melonDS,
as such a vulnerability would most likely affect them as well.

If you are able to provide a fix,
please do so in the form of a pull request or a patch file;
I will merge and release it as soon as possible.
