QTA = g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++
QTB = g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++

all: Publicite Consultation Modification CreationBD Serveur Administrateur Client

Publicite:	Publicite.cpp
		g++ -o Publicite Publicite.cpp

Consultation:	Consultation.cpp
		g++ Consultation.cpp -o Consultation -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

Modification:	Modification.cpp
		g++ Modification.cpp -o Modification -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

CreationBD:	CreationBD.cpp
		g++ -o CreationBD CreationBD.cpp -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

Serveur:	Serveur.cpp Login.o
		g++ Serveur.cpp Login.o -o Serveur -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

Administrateur:	mainAdmin.o windowadmin.o  moc_windowadmin.o
		g++  -o Administrateur mainAdmin.o windowadmin.o moc_windowadmin.o   /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread

Client:	dialogmodification.o mainClient.o windowclient.o moc_dialogmodification.o moc_windowclient.o
		g++  -o Client dialogmodification.o mainClient.o windowclient.o moc_dialogmodification.o moc_windowclient.o   /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread


mainAdmin.o:	mainAdmin.cpp
		$(QTA) -o mainAdmin.o mainAdmin.cpp

windowadmin.o: windowadmin.cpp
		$(QTA) -o windowadmin.o windowadmin.cpp

moc_windowadmin.o: moc_windowadmin.cpp
		$(QTA) -o moc_windowadmin.o moc_windowadmin.cpp

dialogmodification.o: dialogmodification.cpp
		$(QTB) -o dialogmodification.o dialogmodification.cpp

mainClient.o:	mainClient.cpp
		$(QTB) -o mainClient.o mainClient.cpp

windowclient.o: windowclient.cpp
		$(QTB) -o windowclient.o windowclient.cpp

moc_dialogmodification.o:	moc_dialogmodification.cpp
		$(QTB) -o moc_dialogmodification.o moc_dialogmodification.cpp

moc_windowclient.o: moc_windowclient.cpp
		$(QTB) -o moc_windowclient.o moc_windowclient.cpp

Login.o:	Login.cpp
		g++ -c Login.cpp -o Login.o