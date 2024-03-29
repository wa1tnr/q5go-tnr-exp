/*
*   msg_handler.cpp
*/

#include "msg_handler.h"


#ifdef OWN_DEBUG_MODE
void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
	if (view == nullptr || myapp.startingUp () || myapp.closingDown ())
		return;

	//QString msg2 = QString::QString(msg);

	switch (type)
	{
	case QtInfoMsg:
		view->setTextColor (Qt::darkGreen);
		view->append("Info: "  + msg);
		break;
	case QtDebugMsg:
		view->setTextColor (Qt::black);
		view->append("Debug: "  + msg);
		break;
	case QtWarningMsg:
		view->setTextColor (Qt::darkYellow);
		view->append((QString) "Warning: " + msg);
		break;
	case QtCriticalMsg:
		view->setTextColor (Qt::darkRed);
		view->append((QString) "Critical: " + msg);
		break;
	case QtFatalMsg:
		view->setTextColor (Qt::red);
		view->append((QString) "Fatal: " + msg);
		break;
	}
}
#endif
