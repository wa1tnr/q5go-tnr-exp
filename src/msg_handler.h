/*
 *   msg_handler.h
 */

#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#undef OWN_DEBUG_MODE
// comment following message if no "-debug" command allowed
// DON'T DO IT - THERE'S A PROBLEM I MUST SOLVE SOON ... but now it works fine
#define OWN_DEBUG_MODE



#include <QApplication>
#include <QTextEdit>

void myMessageHandler(QtMsgType, const QMessageLogContext&, const QString&);

#ifdef OWN_DEBUG_MODE
extern QTextEdit *view;
extern QApplication myapp;
#endif

#endif
