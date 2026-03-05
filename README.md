# blocktimer

`blocktimer` is a Linux firewall scheduler that blocks domains, IPs, and CIDR subnets during configured time windows by managing `iptables` and `ip6tables` rules.

## Dependency Requirements

### Build Dependencies

- C compiler with C11/gnu11 support (GCC or Clang)
- Meson
- Ninja
- One of:
  - `libelogind`
  - `libsystemd` (provides `sd-bus`)

### Runtime Dependencies

- Linux with netfilter support
- `iptables`
- `ip6tables`
- `systemd-logind` or `elogind` (wake/sleep event handling)

## Build & Install

### Build

```bash
meson setup build --buildtype=debug
ninja -C build
```

### Install Binary

```bash
sudo ninja -C build install
```

### Optional Meson Install Flags

- `-Dsystemd=true|false` install systemd service (default: `true` if systemd is detected)
- `-Dopenrc=true|false` install OpenRC script (default: `false`)
- `-Dconf=true|false` install `block.conf` template (default: `false`)

Example:

```bash
meson setup build \
  --prefix=/usr \
  --sysconfdir=/etc \
  -Dsystemd=true \
  -Dopenrc=false \
  -Dconf=true

ninja -C build
sudo ninja -C build install
```

## Usage

Run as root for real firewall changes:

```bash
sudo blocktimer
```

Run without root for dry-run command output:

```bash
blocktimer
```

Service management (systemd):

```bash
sudo systemctl enable --now blocktimer
sudo systemctl status blocktimer
```

## `block.conf` Syntax

`blocktimer` reads `./block.conf` first, then `/etc/blocktimer/block.conf`.

Each `[block]` section supports:

- `start = HH:MM` start time (24-hour format)
- `stop = HH:MM` stop time (24-hour format)
- `domain += <domain>` domain entry (repeatable)
- `ipv4 += <IPv4-or-CIDR>` IPv4 host/subnet entry (repeatable)
- `ipv6 += <IPv6-or-CIDR>` IPv6 host/subnet entry (repeatable)
- `skipdays = [1,2,...,7]` optional skip days (`1=Mon ... 7=Sun`)

Notes:

- `start == stop` means 24/7 blocking
- `start > stop` means block spans midnight
- A `[block]` must contain at least one of: `domain`, `ipv4`, or `ipv6`

Example:

```ini
[block]
start = 09:00
stop = 17:00
domain += youtube.com
ipv4 += 1.1.1.1
ipv4 += 10.0.0.0/24
ipv6 += 2606:4700:4700::1111
ipv6 += 2001:db8::/64
skipdays = [6,7]
```
