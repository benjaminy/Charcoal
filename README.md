Charcoal
========

Reactive/Interactive Programming Done Right

See http://charcoal-lang.org/

Quick notes on building:

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

