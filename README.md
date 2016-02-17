Bastardo doesn't use the syscall table.

It uses the VFS layer to inject its hooks.

Therefore, it is stealthier.

Features:
* local escalation.
* kernel module hiding
* process hiding

TODO:
* remote escalation with callback
* full directory hiding
* network stealth capabilities
* reboot resiliency
