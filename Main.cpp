#include <stdio.h>
#include <QtCore/QtCore>
#include <QtWidgets/QApplication>
#include "GUIMain.h"

int main(int argc, char *argv[]) {
	printf("Qt Version: %s\n", qVersion() );

	QApplication app(argc, argv);
	GUIMain win;

	return app.exec();
}