Charcoal
========

Reactive/Interactive Programming Done Right

See http://charcoal-lang.org/

Quick notes on building:

cd ThirdParty/libuv/Releases/1.0.0
sh autogen.sh
./configure --prefix=$CHARCOAL_HOME/Install
make && make check && make install

cd ThirdParty/zlog/Charcoal-1.2.12/
make PREFIX=$CHARCOAL_HOME/Install install

cd ThirdParty/OpenPA/Releases/1.0.4/
./configure --prefix=$CHARCOAL_HOME/Install
make && make install

cd ThirdParty/cil/Charcoal-1.7.3/
./configure --prefix=$CHARCOAL_HOME/Install
make && make test && make install

cd Source
make all

## Install Go

bash < <(curl -s -S -L https://raw.githubusercontent.com/moovweb/gvm/master/binscripts/gvm-installer)

[[ -s "$HOME/.gvm/scripts/gvm" ]] && source "$HOME/.gvm/scripts/gvm"

Logout

sudo apt-get install bison

gvm version

gvm install go1.4

gvm listall

gvm use go1.4

gvm install go1.5.3

### Running Go Programs

export GOPATH=$( pwd )

go 