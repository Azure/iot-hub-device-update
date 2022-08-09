# build on rpi3 raspbian bullseye deb 11
# if on buster first upgrade to bullseye

sudo apt update
sudo apt install git
mkdir src
cd src
git clone https://github.com/sbabic/swupdate

sudo apt -y install build-essential
sudo apt -y install lua5.2
sudo apt -y install liblua5.2-dev
sudo apt -y install libconfig-dev
sudo apt -y install libarchive-dev
sudo apt -y install libcurses-dev
sudo apt -y install libssl-dev
sudo apt -y install zlib1g-dev
sudo apt -y install zstd
sudo apt -y install libzstd-dev
sudo apt -y install libubootenv-dev

sudo ln -s /usr/lib/arm-linux-gnueabihf/pkgconfig/lua5.2.pc /usr/lib/pkgconfig/lua.pc

export PKG_CONFIG_PATH=/usr/lib/pkconfig

make
