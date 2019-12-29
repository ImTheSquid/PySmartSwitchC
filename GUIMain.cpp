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


GUIMain::GUIMain() {
	this->setWindowTitle(QString("PySmartSwitch Client"));

	// Generate encryption keys
	printf("Generating RSA keys...\n");
	using namespace CryptoPP;
	// Generate params
	InvertibleRSAFunction params;
	params.GenerateRandomWithKeySize(*rng, 2048);
	// Create keys
	privateKey = new RSA::PrivateKey(params);
	publicKey = new RSA::PublicKey(params);

	ByteQueue queue;
	publicKey->DEREncodePublicKey(queue);

	string tempStr;
	StringSink out(tempStr);
	queue.CopyTo(out);
	out.MessageEnd();
	printf("Keys successfully generated.\n");
	publicStr = new string(tempStr);

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
	connect(socket, &QTcpSocket::readyRead, this, &GUIMain::receiveData);

	sendCommandLayout->addWidget(commandInput);
	sendCommandLayout->addWidget(sendCommandButton);

	mainLayout->addWidget(logBox);

	this->setLayout(mainLayout);

	this->show();

	startConnection();
}

void GUIMain::startConnection() {
	connectionLog->clear();
	connectionStatus->clear();
	connectionStatus->setTextColor(QColor("red"));
	connectionStatus->append("NOT CONNECTED");
	connectionStatus->setTextColor(QColor("black"));
	commandInput->setText("");

	using namespace CryptoPP;
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

		ByteQueue queue2;
		vector dataVec(arr.begin(), arr.end());
		vector<CryptoPP::byte> byteData;
		for (char& c : dataVec) {
			byteData.push_back(static_cast<CryptoPP::byte>(c));
		}
		VectorSource srcString(byteData, true);
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
		return false;
	}
	return true;
}

string GUIMain::encryptData(string data) {
	using namespace CryptoPP;
	RSAES<OAEP<SHA256>>::Encryptor e(*serverPublic);
	string cipher;
	StringSource ss1(data, true, new PK_EncryptorFilter(*rng, e, new StringSink(cipher)));
	return cipher;
}

string GUIMain::decryptData(string cipher) {
	using namespace CryptoPP;
	RSAES<OAEP<SHA256>>::Decryptor d(*privateKey);
	string recovered;
	StringSource ss2(cipher, true, new PK_DecryptorFilter(*rng, d, new StringSink(recovered)));
	return recovered;
}

void GUIMain::sendCommand() {
	appendLog(commandInput->text());
	string encrypted = encryptData(commandInput->text().toStdString());
	socket->write(QByteArray(encrypted.c_str(), encrypted.length()));
	socket->flush();
	commandInput->setText("");
}

void GUIMain::receiveData() {
	if (!connectionReady) return;
	QByteArray byteRecv = socket->read(2048);
	string recv(byteRecv.constData(), 256);
	string decrypted = decryptData(recv);
	appendLog(QString::fromStdString(decrypted), true);
	if (decrypted.compare("EXIT") == 0) {
		socket->close();
		printf("Connection closed.\n");
		startConnection();
	}
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