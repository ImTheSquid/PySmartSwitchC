#include <QtWidgets/QWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtGui/QKeyEvent>
#include <QtCore/Qt>

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

	void keyPressEvent(QKeyEvent* event) override {
		// Enter key
		if (event->key() == 16777220 && commandInput->text().length() > 0) sendCommand();
	};
};

