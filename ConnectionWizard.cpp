#include "ConnectionWizard.h"
#include <QtCore/Qt>
#include <QtWidgets/QApplication>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QApplication>
#include <QtCore/QMap>
#include <QtWidgets/QInputDialog>
#include <array>
#include <QtCore/QStandardPaths>
#include <iostream>
#include <fstream>
#include <QtCore/QDir>
#include <json11.hpp>
#include <filesystem>
#include <sstream>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

namespace fs = filesystem;
using namespace std;

ConnectionWizard::ConnectionWizard(QWidget* parent) : QDialog(parent) {
	this->setModal(true);
	this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	this->setWindowTitle("New Connection");
	this->setMinimumSize(QSize(200, 200));

	saveList->addItem(QString("New Connection..."));
	saveList->setSelectionMode(QAbstractItemView::SingleSelection);
	saveList->setCurrentRow(0);

	// Import profiles
	QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	QDir dir = QDir(path);
	fs::create_directories(dir.path().toStdString());
	string combinedPath = dir.filePath("profiles.json").toStdString();
	loadProfiles(combinedPath, loadedConnections);

	QHBoxLayout* mainLayout = new QHBoxLayout();

	/* Left side */
	QGroupBox* savedBox = new QGroupBox("Saved");
	QVBoxLayout* savedLayout = new QVBoxLayout();
	savedBox->setLayout(savedLayout);
	savedLayout->addWidget(saveList);
	removeItem->setEnabled(false);
	savedLayout->addWidget(removeItem);

	mainLayout->addWidget(savedBox);
	// Left connections
	QObject::connect(saveList, &QListWidget::clicked, this, &ConnectionWizard::updateListFunctions);
	QObject::connect(removeItem, &QPushButton::clicked, this, &ConnectionWizard::removeSelectedItem);

	/* Right side */
	QGroupBox* connectionBox = new QGroupBox("Connection Details");
	QVBoxLayout* connectLayout = new QVBoxLayout();
	connectionBox->setLayout(connectLayout);

	connectLayout->addWidget(new QLabel("Server IP:"));
	connectLayout->addWidget(address);
	connectLayout->addWidget(new QLabel("Port:"));
	port->setMinimum(0);
	port->setMaximum(65536);
	port->setValue(30000);
	connectLayout->addWidget(port);
	connectLayout->addWidget(new QLabel("Password:"));
	password->setEchoMode(QLineEdit::Password);
	connectLayout->addWidget(password);
	connectLayout->addStretch();

	mainLayout->addWidget(connectionBox);

	QHBoxLayout* connectionOptions = new QHBoxLayout();
	save->setEnabled(false);
	connectionOptions->addWidget(save);
	connectionOptions->addStretch();
	connect->setEnabled(false);
	connectionOptions->addWidget(connect);

	connectLayout->addLayout(connectionOptions);

	// Right connections
	QObject::connect(connect, &QPushButton::clicked, this, &ConnectionWizard::connectToTarget);
	QObject::connect(save, &QPushButton::clicked, this, &ConnectionWizard::saveCurrentConnection);
	QObject::connect(address, &QLineEdit::textChanged, this, &ConnectionWizard::updateSaveConnect);


	this->setLayout(mainLayout);
	this->exec();
}

void ConnectionWizard::updateListFunctions() {
	removeItem->setEnabled(QString::compare(saveList->currentItem()->text(), "New Connection...") != 0);
	if (QString::compare(saveList->currentItem()->text(), "New Connection...") == 0) {
		address->setText("");
		port->setValue(30000);
	}
	else {
		string current = saveList->currentItem()->text().toStdString();
		address->setText(QString::fromStdString(loadedConnections->value(current).at(0)));
		port->setValue(QString::fromStdString(loadedConnections->value(current).at(1)).toInt());
	}
}

void ConnectionWizard::removeSelectedItem() {
	loadedConnections->remove(saveList->currentItem()->text().toStdString());
	saveList->takeItem(saveList->row(saveList->currentItem()));
	updateListFunctions();
}

void ConnectionWizard::saveCurrentConnection() {
	bool ok;
	QString title = QInputDialog::getText(this, "Save Connection", "Input Connection Name:", QLineEdit::Normal, "", &ok, Qt::WindowCloseButtonHint);

	if (!ok || title.isEmpty()) return;

	string titleStr = title.toStdString();
	if (loadedConnections->contains(titleStr)) {
		int appendDigit = 1;
		while (loadedConnections->contains(titleStr + " (" + to_string(appendDigit) + ")")) ++appendDigit;
		(*loadedConnections)[titleStr + " (" + to_string(appendDigit) + ")"] = {address->text().toStdString(), to_string(port->value())};
		saveList->addItem(QString::fromStdString(titleStr + " (" + to_string(appendDigit) + ")"));
	}
	else {
		(*loadedConnections)[titleStr] = { address->text().toStdString(), to_string(port->value()) };
		saveList->addItem(QString::fromStdString(titleStr));
	}
	saveList->setCurrentRow(saveList->count() - 1);
	removeItem->setEnabled(true);
}

void ConnectionWizard::saveAllConnections() {
	QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	QDir dir = QDir(path);
	fs::create_directories(dir.path().toStdString());
	string combinedPath = dir.filePath("profiles.json").toStdString();

	json11::Json json = loadedConnections->toStdMap();

	ofstream profiles(combinedPath, ofstream::out | ofstream::trunc);
	profiles << json.dump() << endl;
	profiles.close();
}

void ConnectionWizard::updateSaveConnect() {
	bool ok = address->text().length() > 0;
	connect->setEnabled(ok);
	save->setEnabled(ok);
}

void ConnectionWizard::closeEvent(QCloseEvent* event) {
	saveAllConnections();
	exit(0);
}

void ConnectionWizard::connectToTarget() {
	saveAllConnections();
	this->hide();
}

void ConnectionWizard::loadProfiles(string inputFile, QMap<string, vector<string>>* dest) {
	ifstream profiles(inputFile);
	stringstream buffer;
	buffer << profiles.rdbuf();
	const string x = buffer.str();
	QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(x).toUtf8());
	QJsonObject json = doc.object();
	for(const QString & key : json.keys()) {
		QJsonValue value = json.value(key);
		QJsonArray arr = value.toArray();
		vector vec = arr.toVariantList().toVector().toStdVector();
		if (vec.size() != 2) {
			printf("ERROR! INVALID FILE. Skipping loading procedure...");
			return;
		}
		vector<string> newVec;
		newVec.push_back(vec.at(0).toString().toStdString());
		newVec.push_back(vec.at(1).toString().toStdString());
		(*dest)[key.toStdString()] = newVec;
		saveList->addItem(key);
	}
}
