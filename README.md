meson setup build --buildtype=debug
meson setup build --buildtype=release

--sysconfdir=/etc

meson setup build \
  --prefix=/usr \
  --sysconfdir=/etc \
  -Dopenrc=true \
  -Dsystemd=false

ninja -C build
sudo ninja -C build install
