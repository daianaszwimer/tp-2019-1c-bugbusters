# Se ejecuta haciendo: sudo sh ./scriptEntrega.sh
# copio las shared lib en /usr/lib van los .so
# git clone https://github.com/sisoputnfrba/tp-2019-1c-bugbusters.git
echo Copiando shared libraries...
cd
git clone https://github.com/sisoputnfrba/so-commons-library.git
# tambien se puede hacer make install sino
cd so-commons-library
make clean
make
cd
#su "$SUDO_USER" -c cp so-commons-library/src/build/libcommons.so /usr/lib
cp so-commons-library/src/build/libcommons.so /usr/lib
cd
cd tp-2019-1c-bugbusters/nuestro_lib/Debug
make clean
make all
echo Compilando kernel...
cd
cd tp-2019-1c-bugbusters/kernel/Debug
make clean
make all
echo Compilando memoria...
cd
cd tp-2019-1c-bugbusters/memoria/Debug
make clean
make all
echo Compilando file system..
cd
cd tp-2019-1c-bugbusters/lfs/Debug
make clean
make all
echo Seteando library path...
#  ldconfig -n directory_with_shared_libraries
# ldd {ejecutable}
# hay que instalar todas las dependenciassS????
# seteo library path
echo  'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2019-1c-bugbusters/nuestro_lib/Debug' >> ~/.bashrc 
echo Setup finalizado!
