#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtCore/QVector>
#include <Json11.hpp>

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
	QMap<QString, std::array<QString, 2>>* loadedConnections = new QMap<QString, std::array<QString, 2>>();

	void closeEvent(QCloseEvent* event) override { exit(0); };

	void updateListFunctions();

	void removeSelectedItem();

	void saveCurrentConnection();

	void updateSaveConnect();

	void saveAllConnections();
};
