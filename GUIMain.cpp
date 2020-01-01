// For some reason this fixes lots of byte ambiguity errors
#define WIN32_LEAN_AND_MEAN

#include "GUIMain.h"
#include "ConnectionWizard.h"
#include <stdio.h>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <ctime>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QAbstractSocket>
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>
#include <Windows.h>
#include <QtWidgets/QLabel>
#include "resource.h"
#include <QtWidgets/QMessageBox>


GUIMain::GUIMain() {
	this->setWindowTitle(QString("PySmartSwitch Client"));
	updateTimer->setInterval(5000);

	// Generate encryption keys
	printf("Generating RSA keys...\n");
	// Generate params
	CryptoPP::InvertibleRSAFunction params;
	params.GenerateRandomWithKeySize(*rng, 2048);
	// Create keys
	privateKey = new CryptoPP::RSA::PrivateKey(params);
	publicKey = new CryptoPP::RSA::PublicKey(params);

	CryptoPP::ByteQueue queue;
	publicKey->DEREncodePublicKey(queue);

	string tempStr;
	CryptoPP::StringSink out(tempStr);
	queue.CopyTo(out);
	out.MessageEnd();
	printf("Keys successfully generated.\n");
	publicStr = new string(tempStr);

	QHBoxLayout* mainLayout = new QHBoxLayout();

	/* Left side */
	// Switch power status
	QVBoxLayout* statusLayout = new QVBoxLayout();
	QGroupBox* toggleBox = new QGroupBox("Power Status");
	// Import images
	QPixmap* pixmapOn = loadImage(IDB_BITMAP1);
	QPixmap* pixmapOff = loadImage(IDB_BITMAP2);
	onIcon =  new QPixmap(pixmapOn->scaled(QSize(180, 180), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	offIcon = new QPixmap(pixmapOff->scaled(QSize(180, 180), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));

	// Image setup
	QVBoxLayout* toggleImageLayout = new QVBoxLayout();
	toggleBox->setLayout(toggleImageLayout);
	hold->setPixmap(*offIcon);
	hold->setAlignment(Qt::AlignCenter);
	toggleImageLayout->addWidget(hold);
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
	connect(sendCommandButton, &QPushButton::clicked, this, &GUIMain::sendCommandFromUI);
	connect(commandInput, &QLineEdit::textChanged, this, &GUIMain::updateSendButton);
	connect(socket, &QTcpSocket::readyRead, this, &GUIMain::receiveData);
	connect(updateTimer, &QTimer::timeout, this, &GUIMain::timerCommand);

	sendCommandLayout->addWidget(commandInput);
	sendCommandLayout->addWidget(sendCommandButton);

	mainLayout->addWidget(logBox);

	this->setLayout(mainLayout);

	this->show();

	startConnection();
}

void GUIMain::startConnection() {
	// Reset everything
	updateTimer->stop();
	connectionLog->clear();
	connectionStatus->clear();
	connectionStatus->setTextColor(QColor("red"));
	connectionStatus->append("NOT CONNECTED");
	connectionStatus->setTextColor(QColor("black"));
	commandInput->setText("");
	hold->setPixmap(*offIcon);

	// Begin server/client initalization
	bool success = false;
	bool encryptionSuccess = false;
	ConnectionWizard* wizard = nullptr;

	while (!encryptionSuccess) {
		connectionReady = false;
		while (!success) {
			wizard = new ConnectionWizard(this);
			success = requestConnection(wizard->getAddress(), wizard->getPort(), socket);
		}
		success = false;

		printf("Connection established to %s:%i. Sending public key...\n", wizard->getAddress().toStdString().c_str(), wizard->getPort());
		socket->write(QByteArray(publicStr->c_str(), publicStr->length()));
		socket->flush();

		socket->waitForReadyRead(1000);

		// Receive and decode public key
		QByteArray arr = socket->read(2048);

		CryptoPP::ByteQueue queue2;
		vector dataVec(arr.begin(), arr.end());
		vector<CryptoPP::byte> byteData;
		for (char& c : dataVec) {
			byteData.push_back(static_cast<CryptoPP::byte>(c));
		}
		CryptoPP::VectorSource srcString(byteData, true);
		srcString.TransferTo(queue2);
		queue2.MessageEnd();

		try {
			serverPublic->Load(queue2);

			if (!serverPublic->Validate(*rng, 3)) {
				printf("ERROR! Invalid key received! Terminating connection...");
				socket->close();
			}
			else {
				printf("Valid public key received. Sending password over encrypted connection...\n");
				string encrypted = encryptData(wizard->getPassword().toStdString());
				QByteArray bytes(encrypted.c_str(), encrypted.length());
				socket->write(bytes);
				socket->flush();

				socket->waitForReadyRead(1000);
				QByteArray byteRecv = socket->read(2048);
				string recv(byteRecv.constData(), 256);
				if (string("NOT_AUTHORIZED").compare(decryptData(recv)) == 0) {
					printf("Incorrect password. Closing connection...\n");
					socket->close();
				}
				else {
					printf("Password correct.\n");
					encryptionSuccess = true;
					connectionReady = true;
					connectionStatus->clear();
					connectionStatus->setTextColor(QColor("green"));
					connectionStatus->append("CONNECTED");
					connectionStatus->setTextColor(QColor("black"));
					connectionStatus->append("IP: " + wizard->getAddress());
					connectionStatus->append("PORT: " + QString::fromStdString(to_string(wizard->getPort())));
				}
			}
		}
		catch (CryptoPP::Exception & e) {
			cout << e.what() << endl;
			exit(1);
		}
	}
}

bool GUIMain::requestConnection(const QString HOST, const int PORT, QTcpSocket* socket) {
	socket->connectToHost(HOST, PORT, QIODevice::ReadWrite);
	if (!socket->waitForConnected(10000)) {
		printf("Error: %s\n", socket->errorString().toStdString().c_str());
		QMessageBox::critical(this, "Connection Error", "Error: " + socket->errorString());
		return false;
	}
	updateTimer->start();
	return true;
}

string GUIMain::encryptData(string data) {
	CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA256>>::Encryptor e(*serverPublic);
	string cipher;
	CryptoPP::StringSource ss1(data, true, new CryptoPP::PK_EncryptorFilter(*rng, e, new CryptoPP::StringSink(cipher)));
	return cipher;
}

string GUIMain::decryptData(string cipher) {
	CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA256>>::Decryptor d(*privateKey);
	string recovered;
	CryptoPP::StringSource ss2(cipher, true, new CryptoPP::PK_DecryptorFilter(*rng, d, new CryptoPP::StringSink(recovered)));
	return recovered;
}

void GUIMain::sendCommandFromUI() {
	sendCommand(commandInput->text());
}

void GUIMain::sendCommand(QString text, bool bAppendLog, bool clearCommand) {
	if (text.compare("halt") == 0) {
		string encrypted = encryptData("exit");
		socket->write(QByteArray(encrypted.c_str(), encrypted.length()));
		socket->flush();
		socket->abort();
		printf("Connection aborted.\n");
		startConnection();
		return;
	}
	if (bAppendLog) appendLog(text);
	string encrypted = encryptData(text.toStdString());
	socket->write(QByteArray(encrypted.c_str(), encrypted.length()));
	socket->flush();
	if (clearCommand) commandInput->setText("");
}

void GUIMain::timerCommand() {
	// Socket safety check
	if (!socket->isOpen()) return;

	sendCommand("$update", false, false);
}

void GUIMain::receiveData() {
	if (!connectionReady) return;
	QByteArray byteRecv = socket->read(2048);
	string recv(byteRecv.constData(), 256);
	string decrypted = decryptData(recv);
	// Don't log if data query
	if (decrypted.length() > 0 && decrypted.substr(0, 1).compare("$") == 0) {
		istringstream iss(decrypted);
		vector<string> cmdVec((istream_iterator<string>(iss)), istream_iterator<string>());
		string cmd = cmdVec.at(0);
		if (cmd.compare("$EXIT") == 0) {
			socket->close();
			printf("Connection closed.\n");
			startConnection();
		}
		else if (cmd.compare("$POWER") == 0 && cmdVec.size() == 2) {
			hold->setPixmap(cmdVec.at(1).compare("True") == 0 ? *onIcon : *offIcon);
			// Exception to "no log" policy
			if (cmdVec.at(1).compare("True") == 0) appendLog("Relay status set to ON", true);
			else appendLog("Relay status set to OFF", true);
		}
		else if (cmd.compare("$UPDATE") == 0 && cmdVec.size() == 2) {
			// May add some more stuff here later
			hold->setPixmap(cmdVec.at(1).compare("True") == 0 ? *onIcon : *offIcon);
		}
	}
	else appendLog(QString::fromStdString(decrypted), true);
}

void GUIMain::updateSendButton() {
	sendCommandButton->setEnabled(commandInput->text().length() > 0);
}

void GUIMain::appendLog(const QString text, bool isServer) {
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
	out.append(direction.c_str() + text);
	connectionLog->append(out);
}

void GUIMain::closeEvent(QCloseEvent* event) {
	if (socket->isOpen()) {
		string encrypted = encryptData("exit");
		socket->write(QByteArray(encrypted.c_str(), encrypted.length()));
		socket->flush();
		socket->close();
	}
	exit(0);
}

QPixmap* GUIMain::loadImage(int name) {
	// Resource loading
	HRSRC rc = FindResource(NULL, MAKEINTRESOURCE(name), RT_RCDATA);
	
	if (rc == NULL) {
		printf("INVALID RESOURCE ADDRESS (%i)\n", name);
		return new QPixmap();
	}
	
	HGLOBAL rcData = LoadResource(NULL, rc);
	LPVOID data = LockResource(rcData);
	DWORD data_size = SizeofResource(NULL, rc);

	// Rewrite file to new file, testing only
	/*
	ofstream output("E:\\" + to_string(name) + ".bmp", std::ios::binary);
	// rc.exe strips header information, need to restore it
	BITMAPFILEHEADER bfh = { 'MB', 54 + data_size, 0, 0, 54 };
	output.write((char*)&bfh, sizeof(bfh));
	output.write((char*)data, data_size);
	output.close();
	*/

	// Load into pixmap
	QPixmap* pm = new QPixmap();
	pm->loadFromData(static_cast<uchar*>(data), data_size, "bmp");
	
	return pm;
}