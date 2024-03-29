/*
* textview.h
*/

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <QtSvg>

#include "ui_textview_gui.h"
#include "globals.h"

class TextView : public QDialog, public Ui::TextViewGUI
{
	Q_OBJECT

public:
	enum class type { exprt, gtp };
	TextView(QWidget* parent = 0, type m = type::exprt);
	~TextView();
	void append (const QString &);
	void set (const QString &);

public slots:
	void toClipboard();
	void saveMe();
};

#include "ui_svgview_gui.h"

class SvgWidget: public QSvgWidget
{
	QWidget *m_container;
public:
	SvgWidget (QWidget *container) : m_container (container)
	{
	}
	void fix_aspect ();
	virtual void resizeEvent (QResizeEvent *)
	{
		fix_aspect ();
	}
};

class SvgView : public QDialog, public Ui::SvgViewGUI
{
	Q_OBJECT
	QString m_svg;
	SvgWidget *m_view {};
public:
	SvgView(QWidget* parent = 0);
	~SvgView();
	void set (const QByteArray &);

public slots:
	void toClipboard();
	void saveMe();
};

#endif // TEXTVIEW_H
