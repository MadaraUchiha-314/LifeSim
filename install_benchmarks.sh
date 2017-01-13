cd HotSpot-6.0/
make 
cd ..
wget http://snipersim.org/packages/sniper-benchmarks.tbz
tar xjf sniper-benchmarks.tbz
rm sniper-benchmarks.tbz
cp verify_benchmark_installation.sh ./benchmarks
mkdir temporary_perl
cd temporary_perl
wget launchpadlibrarian.net/137483681/perl_5.14.2-21_amd64.deb
sudo dpkg --force-all -i perl*
rm *
cd ..
rmdir temporary_perl
cp restrict-threads.patch ./benchmarks
cd benchmarks
make -C splash2
# make -C parsec
patch < restrict-threads.patch
sudo apt-get install perl