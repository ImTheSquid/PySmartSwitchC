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

ConnectionWizard::ConnectionWizard(QWidget* parent) : QDialog(parent) {
	this->setModal(true);
	this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	this->setWindowTitle("New Connection");

	saveList->addItem(QString("New Connection..."));
	saveList->setSelectionMode(QAbstractItemView::SingleSelection);
	saveList->setCurrentRow(0);

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
	QObject::connect(connect, &QPushButton::clicked, this, &QDialog::hide);
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
		QString current = saveList->currentItem()->text();
		address->setText(loadedConnections->value(current)[0]);
		port->setValue(loadedConnections->value(current)[1].toInt());
	}
}

void ConnectionWizard::removeSelectedItem() {
	
}

void ConnectionWizard::saveCurrentConnection() {
	bool ok;
	QString title = QInputDialog::getText(this, "Save Connection", "Input Connection Name:", QLineEdit::Normal, "", &ok, Qt::WindowCloseButtonHint);

	if (!ok || title.isEmpty()) return;
}

void ConnectionWizard::saveAllConnections() {

}

void ConnectionWizard::updateSaveConnect() {
	bool ok = address->text().length() > 0;
	connect->setEnabled(ok);
	save->setEnabled(ok);
}
