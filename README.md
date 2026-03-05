# Blocktimer

A time-based domain blocking utility for Linux that uses iptables/ip6tables to block network access to specified domains on a configurable schedule.

## Features

- **Time-based blocking**: Define start/stop times (HH:MM) for each domain group
- **Day-based skipping**: Skip blocking on specific days of the week
- **24/7 blocking support**: Optionally block domains around the clock
- **Multi-threaded scheduler**: Independent threads for each block unit with adaptive sleep times
- **IPv4 & IPv6 support**: Uses both iptables and ip6tables simultaneously
- **Timezone-aware**: Uses system timezone for all time calculations
- **Graceful shutdown**: Automatic firewall cleanup on exit
- **Non-root mode**: Can run without root privileges (prints commands instead of executing)

## Requirements

### Build Dependencies

- **C Compiler**: GCC 9+ or Clang
- **Meson**: Build system (>= 0.63)
- **Ninja**: Build tool
- **D-Bus library**: One of:
  - `libsystemd` (systemd systems)
  - `libelogind` (elogind systems, checked first)
- **POSIX threads** (pthread - included in glibc)

### Runtime Dependencies

- **Linux kernel** with netfilter support
- **iptables** and **ip6tables** (for firewall rules)
- **systemd-logind** or **elogind** (for D-Bus system wake-up events)

### Package Installation

**Debian/Ubuntu:**
```bash
sudo apt install build-essential meson ninja-build libsystemd-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc meson ninja-build systemd-devel
```

**Gentoo:**
```bash
emerge dev-lang/gcc dev-util/meson dev-util/ninja sys-apps/systemd
```

**Arch:**
```bash
sudo pacman -S base-devel meson ninja systemd
```

## Building

### Debug Build (with enhanced logging)

```bash
meson setup build --buildtype=debug
ninja -C build
```

Debug builds enable detailed logging with file and line numbers. Output appears on stderr.

### Release Build (optimized)

```bash
meson setup build --buildtype=release
ninja -C build
```

## Installation

### System-wide Installation

Install with systemd service file (default):
```bash
meson setup build \
  --prefix=/usr \
  --sysconfdir=/etc \
  -Dsystemd=true \
  -Dopenrc=false

ninja -C build
sudo ninja -C build install
```

Install with OpenRC init script:
```bash
meson setup build \
  --prefix=/usr \
  --sysconfdir=/etc \
  -Dopenrc=true \
  -Dsystemd=false

ninja -C build
sudo ninja -C build install
```

Install without init scripts:
```bash
meson setup build \
  --prefix=/usr \
  --sysconfdir=/etc \
  -Dsystemd=false \
  -Dopenrc=false

ninja -C build
sudo ninja -C build install
```

Install configuration file template:
```bash
meson setup build \
  --prefix=/usr \
  --sysconfdir=/etc \
  -Dconf=true

ninja -C build
sudo ninja -C build install
```

### Uninstall

```bash
sudo ninja -C build uninstall
```

## Configuration

### Configuration File

The configuration file is located at `/etc/blocktimer/block.conf`. It defines one or more blocking rules with time windows and domain lists.

### Configuration Syntax

```ini
[block]
# Start time in 24-hour format (HH:MM)
start = 14:00

# Stop time in 24-hour format (HH:MM)
stop = 10:00

# Add domains to block (one per line, use += to add multiple)
domain += hamsterpaj.net
domain += example.com

# Optional: Days to skip (1-7 = Monday-Sunday)
skipdays = [1,7]
```

### Configuration Examples

**Block YouTube from 9 AM to 5 PM on weekdays:**
```ini
[block]
start = 09:00
stop = 17:00
domain += youtube.com
skipdays = [6,7]  # Skip Saturday and Sunday
```

**Block example.com 24/7:**
```ini
[block]
start = 00:00
stop = 00:00  # When start == stop, blocked all day
domain += example.com
```

**Block multiple domains with different schedules:**
```ini
[block]
start = 00:00
stop = 07:00
domain += youtube.com
domain += twitter.com

[block]
start = 20:00
stop = 22:00
domain += discord.com
```

**Block that spans midnight (9 PM to 7 AM):**
```ini
[block]
start = 21:00
stop = 07:00  # Blocks 21:00-23:59 and 00:00-07:00
domain += gaming-site.com
```

**Skip blocking on specific days:**
```ini
[block]
start = 10:00
stop = 18:00
domain += social-media.com
skipdays = [5,6,7]  # Skip Friday, Saturday, Sunday
```

### Configuration Notes

- **Time Format**: 24-hour format (00:00 to 23:59)
- **Domain Resolution**: Domains are resolved to IP addresses and added to firewall rules
- **Midnight Wraparound**: If `start > stop`, blocking spans midnight (e.g., 22:00 to 06:00)
- **24/7 Blocking**: When `start == stop`, domains are blocked all day (IPs updated at that time)
- **Day Skipping**: When a skip day matches, the entire day is skipped - no filtering on that day
- **IP Updates**: IP addresses are refreshed at configured start/stop times
- **IPv6**: Both IPv4 and IPv6 addresses are resolved and blocked

## Usage

### Starting the Service

**systemd:**
```bash
sudo systemctl start blocktimer
sudo systemctl enable blocktimer  # Enable at boot
```

**OpenRC:**
```bash
sudo rc-service blocktimer start
sudo rc-update add blocktimer    # Enable at boot
```

**Manual:**
```bash
sudo blocktimer
```

### Monitoring

**View systemd logs:**
```bash
sudo journalctl -u blocktimer -f
```

**In debug builds without init system:**
```bash
sudo blocktimer 2>&1 | tee blocktimer.log
```

### Testing (without root)

To test configuration without applying firewall rules:
```bash
blocktimer
# Will print intended iptables commands instead of executing them
```

### Stopping the Service

**systemd:**
```bash
sudo systemctl stop blocktimer
```

**OpenRC:**
```bash
sudo rc-service blocktimer stop
```

**Manual (Ctrl+C):**
```bash
sudo blocktimer
# Press Ctrl+C to stop
```

## How It Works

1. **Configuration Parsing**: Reads block definitions with time windows and domain lists
2. **DNS Resolution**: Resolves each domain to IPv4 and IPv6 addresses
3. **Firewall Setup**: Creates custom `blocktimer` iptables chain in OUTPUT and FORWARD rules
4. **Scheduling**: For each block unit, a thread:
   - Checks if today is a skip day
   - Determines if current time falls within block window
   - Adds/removes IP addresses from firewall rules accordingly
   - Calculates sleep time until next state change
5. **System Wake-up**: Listens for system wake events via D-Bus (systemd-logind/elogind)
6. **Graceful Shutdown**: On SIGINT/SIGTERM, removes all rules and exits cleanly

## Firewall Rules

Blocktimer creates a custom iptables chain named `blocktimer` and inserts rules into:
- **IPv4**: OUTPUT and FORWARD chains
- **IPv6**: OUTPUT and FORWARD chains (ip6tables)

Blocked traffic is matched by destination IP address, not by domain name. The application resolves domains to IPs and maintains the rules.

## Troubleshooting

**"Permission denied" when installing:**
- Installation requires root. Use `sudo ninja -C build install`

**"Neither libelogind nor libsystemd found":**
- Install libsystemd: `sudo apt install libsystemd-dev` (Debian/Ubuntu)
- Or install libelogind: `sudo apt install libelogind-dev`

**Firewall rules not being removed on exit:**
- Press Ctrl+C again to force exit
- Manually remove rules: `sudo iptables -D OUTPUT -j blocktimer` and `sudo iptables -F blocktimer && iptables -X blocktimer`

**Configuration not taking effect:**
- Reload: `sudo systemctl restart blocktimer`
- Check syntax: `sudo blocktimer` (without systemd) and look for parse errors
- Verify paths: Config should be at `/etc/blocktimer/block.conf`

**DNS resolution failing:**
- Check system DNS: `nslookup example.com`
- Verify network connectivity
- Check logs for "ERROR_ADDRINFO" messages

## Development

See `AGENTS.md` for guidelines on working with this codebase.

### Building for Development

```bash
meson setup build --buildtype=debug
ninja -C build
./build/blocktimer  # Run directly
```

### Code Organization

- **src/main.c**: Core scheduler and threading
- **src/blocktimer.h**: Core data structures
- **src/parse_config.c**: Configuration file parsing
- **src/addr.c**: Firewall rule management (iptables integration)
- **src/fetch_addresses.c**: DNS resolution
- **src/waitforwakeup.c**: D-Bus system wake-up monitoring
- **src/libmemhandle/**: Custom memory management library

## License

See LICENSE file for details.

## Contributing

To contribute, please ensure:
1. Code follows `-Wpedantic` compliance
2. Changes are tested in both debug and release builds
3. Firewall rules are properly tested (may require root/VM environment)
4. Configuration file changes are documented
