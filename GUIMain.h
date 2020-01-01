#include <QtWidgets/QWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtGui/QKeyEvent>
#include <QtCore/Qt>
#include <QtCore/QString>
#include <QtNetwork/QTcpSocket>
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <Windows.h>
#include "resource.h"
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>

#pragma once
class GUIMain :
	public QWidget
{
public:
	GUIMain();

	~GUIMain() {
		DeleteObject(MAKEINTRESOURCE(IDB_BITMAP1));
		DeleteObject(MAKEINTRESOURCE(IDB_BITMAP2));
	};

private:
	QTextEdit* connectionStatus = new QTextEdit();
	QPixmap* onIcon = nullptr;
	QPixmap* offIcon = nullptr;
	QTextEdit* connectionLog = new QTextEdit();
	QLineEdit* commandInput = new QLineEdit();
	QPushButton* sendCommandButton = new QPushButton("Send");
	QTcpSocket* socket = new QTcpSocket();
	QLabel* hold = new QLabel("POWER_ICON");
	QTimer* updateTimer = new QTimer(this);

	CryptoPP::AutoSeededRandomPool* rng = new CryptoPP::AutoSeededRandomPool();
	CryptoPP::RSA::PrivateKey* privateKey = nullptr;
	CryptoPP::RSA::PublicKey* publicKey = nullptr;
	CryptoPP::RSA::PublicKey* serverPublic = new CryptoPP::RSA::PublicKey();
	std::string* publicStr;
	bool connectionReady = false;

	void startConnection();

	void sendCommandFromUI();

	void sendCommand(QString text, bool bAppendLog = true, bool clearCommand = true);

	// Sends keep alive/update request
	void timerCommand();

	void receiveData();

	void updateSendButton();

	void appendLog(const QString text, const bool isServer = false);

	void keyPressEvent(QKeyEvent* event) override {
		// Enter key
		if (event->key() == 16777220 && commandInput->text().length() > 0) sendCommand(commandInput->text());
	};

	bool requestConnection(const QString HOST, const int PORT, QTcpSocket* socket);

	std::string encryptData(std::string data);

	std::string decryptData(std::string cipher);

	void closeEvent(QCloseEvent* event) override;

	QPixmap* loadImage(int name);
};

