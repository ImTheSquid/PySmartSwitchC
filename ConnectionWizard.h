#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtCore/QVector>
#include <Json11.hpp>
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;

#pragma once
class ConnectionWizard :
	public QDialog
{
public:
	ConnectionWizard(QWidget* parent);

	int getPort() { return port->value(); };

	QString getAddress() { return address->text(); }

	QString getPassword() { return password->text(); }

private:
	QListWidget* saveList = new QListWidget();
	QPushButton* removeItem = new QPushButton("Remove Selected Item");
	QLineEdit* address = new QLineEdit();
	QSpinBox* port = new QSpinBox();
	QLineEdit* password = new QLineEdit();
	QPushButton* connect = new QPushButton("Connect");
	QPushButton* save = new QPushButton("Save");
	QMap<string, vector<string>>* loadedConnections = new QMap<string, vector<string>>();

	void closeEvent(QCloseEvent* event) override;

	void updateListFunctions();

	void removeSelectedItem();

	void saveCurrentConnection();

	void updateSaveConnect();

	void saveAllConnections();

	void connectToTarget();

	void loadProfiles(string inputFile, QMap<string, vector<string>>* dest);
};
