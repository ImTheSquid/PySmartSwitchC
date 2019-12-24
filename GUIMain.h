#include <QtWidgets/QWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#pragma once
class GUIMain :
	public QWidget
{
public:
	GUIMain();

private:
	QTextEdit* connectionStatus = new QTextEdit();
	QTextEdit* connectionLog = new QTextEdit();
	QLineEdit* commandInput = new QLineEdit();
	QPushButton* sendCommandButton = new QPushButton("Send");

	void sendCommand();

	void updateSendButton();

	void appendLog(bool isServer = false);
};

