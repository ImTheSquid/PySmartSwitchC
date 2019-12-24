#include "GUIMain.h"
#include "ConnectionWizard.h"
#include <stdio.h>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <ctime>

GUIMain::GUIMain() {
	this->setWindowTitle(QString("PySmartSwitch Client"));

	QHBoxLayout* mainLayout = new QHBoxLayout();

	/* Left side */
	// Switch power status
	QVBoxLayout* statusLayout = new QVBoxLayout();
	QGroupBox* toggleBox = new QGroupBox("Power Status");
	toggleBox->setFixedSize(QSize(200, 200));
	statusLayout->addWidget(toggleBox);

	// Switch connection status
	QGroupBox* statusBox = new QGroupBox("Connection Status");
	QVBoxLayout* statBoxLayout = new QVBoxLayout();
	connectionStatus->setReadOnly(true);
	statBoxLayout->addWidget(connectionStatus);

	statusBox->setLayout(statBoxLayout);
	statusBox->setFixedWidth(200);
	statusLayout->addWidget(statusBox);

	mainLayout->addLayout(statusLayout);


	/* Right side */
	// Log box
	QGroupBox* logBox = new QGroupBox();
	QVBoxLayout* logLayout = new QVBoxLayout();
	logBox->setLayout(logLayout);
	connectionLog->setReadOnly(true);
	logLayout->addWidget(connectionLog);

	// Command sender
	QHBoxLayout* sendCommandLayout = new QHBoxLayout();
	logLayout->addLayout(sendCommandLayout);
	commandInput->setPlaceholderText("Enter command");
	sendCommandButton->setEnabled(false);
	// Connections
	connect(sendCommandButton, &QPushButton::clicked, this, &GUIMain::sendCommand);
	connect(commandInput, &QLineEdit::textChanged, this, &GUIMain::updateSendButton);
	sendCommandLayout->addWidget(commandInput);
	sendCommandLayout->addWidget(sendCommandButton);

	mainLayout->addWidget(logBox);

	this->setLayout(mainLayout);

	this->show();

	ConnectionWizard* wizard = new ConnectionWizard(this->parentWidget());
}

void GUIMain::sendCommand() {
	appendLog();
	commandInput->setText("");
}

void GUIMain::updateSendButton() {
	sendCommandButton->setEnabled(commandInput->text().length() > 0);
}

void GUIMain::appendLog(bool isServer) {
	// Get time
	std::time_t rawtime = time(0);
	char dt[26];
	ctime_s(dt, sizeof(dt), &rawtime);
	std::string str(dt);
	str.erase(std::find(str.begin(), str.end(), '\n'), str.end());

	QString out("[");
	out.append(QString(str.c_str()) + "]");
	std::string direction;
	direction.append(isServer ? "<" : ">");
	out.append(direction.c_str() + commandInput->text());
	connectionLog->append(out);
}