/*
* mainwidget.cpp
*/

#include <QPixmap>
#include <QGraphicsTextItem>

#include "qgo.h"
#include "mainwidget.h"
#include "defines.h"
#include <qpushbutton.h>
#include <qlayout.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtabwidget.h>

ClockView::ClockView (QWidget *parent)
	: QGraphicsView (parent)
{
	QFontMetrics m (setting->fontClocks);
	int w = width ();
	int h = m.height ();
	setMinimumHeight (h);
	setMaximumHeight (h);
	m_scene = new QGraphicsScene (0, 0, w, h, this);
	setScene (m_scene);
	m_text = m_scene->addText ("00:00", setting->fontClocks);
	m_text->setDefaultTextColor (Qt::white);
	update_pos ();
}

void ClockView::resizeEvent (QResizeEvent *)
{
	update_pos ();
}

void ClockView::mousePressEvent (QMouseEvent *e)
{
	if (e->button () == Qt::LeftButton)
		emit clicked ();
}

void ClockView::update_pos ()
{
	QRectF br = m_text->boundingRect ();
	m_text->setPos ((width () - br.width ()) / 2, (height () - br.height ()) / 2);
	m_scene->update ();
}

void ClockView::update_font (const QFont &f)
{
	m_text->setFont (f);
	QFontMetrics m (f);
	int h = m.height ();
	setMinimumHeight (h);
	setMaximumHeight (h);

	update_pos ();
}

void ClockView::set_text (const QString &s)
{
	m_text->setPlainText (s);
	update_pos ();
}

void ClockView::flash (bool on)
{
	setBackgroundBrush (QBrush (on ? Qt::red : Qt::black));
	m_text->setDefaultTextColor (on ? Qt::black : Qt::white);
}


/*
 *  Constructs a MainWidget which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
MainWidget::MainWidget(MainWindow *win, QWidget* parent)
	: QWidget (parent), m_mainwin (win)
{
	setupUi(this);
	gfx_board->init2 (win, this);

	void (QTabWidget::*changed) (int) = &QTabWidget::currentChanged;
	connect(toolsTabWidget, changed, this, &MainWidget::slot_toolsTabChanged);

	connect(passButton, SIGNAL(clicked()), win, SLOT(doPass()));
        connect(undoButton, SIGNAL(clicked()), win, SLOT(doUndo()));
        connect(adjournButton, SIGNAL(clicked()), win, SLOT(doAdjourn()));
        connect(resignButton, SIGNAL(clicked()), win, SLOT(doResign()));
        connect(doneButton, SIGNAL(clicked()), win, SLOT(doCountDone()));

	normalTools->show();
	scoreTools->hide();

	showSlider = true;
	toggleSlider(setting->readBoolEntry("SLIDER"));
	slider->setMaximum(SLIDER_INIT);
	sliderRightLabel->setText(QString::number(SLIDER_INIT));
	sliderSignalToggle = true;

	updateFont ();

	int w = evalView->width ();
	int h = evalView->height ();

	m_eval_canvas = new QGraphicsScene (0, 0, w, h, evalView);
	evalView->setScene (m_eval_canvas);
	m_eval_bar = m_eval_canvas->addRect (QRectF (0, 0, w, h / 2), Qt::NoPen, QBrush (Qt::black));

	m_eval = 0.5;
	connect (evalView, &SizeGraphicsView::resized, this, [=] () { set_eval (m_eval); });
}

void MainWidget::update_game_record (std::shared_ptr<game_record> gr)
{
	m_game = gr;
	if (gr->ranked_type () == ranked::free)
		normalTools->TextLabel_free->setText(QApplication::tr("free"));
	else if (gr->ranked_type () == ranked::ranked)
		normalTools->TextLabel_free->setText(QApplication::tr("rated"));
	else
		normalTools->TextLabel_free->setText(QApplication::tr("teach"));
	normalTools->komi->setText(QString::number(gr->komi ()));
	scoreTools->komi->setText(QString::number(gr->komi ()));
	normalTools->handicap->setText(QString::number(gr->handicap ()));
}

void MainWidget::init_game_record (std::shared_ptr<game_record> gr)
{
	update_game_record (gr);
	gfx_board->reset_game (gr);
}

/*
*  Destroys the object and frees any allocated resources
*/
MainWidget::~MainWidget()
{
	// no need to delete child widgets, Qt does it all for us
	delete scoreTools;
	delete normalTools;
}

// a tab has been clicked
void MainWidget::slot_toolsTabChanged(int idx)
{
	Board *b = gfx_board;
	switch (idx)
	{
		// normal/score tools
	case tabNormalScore:
		b->setMode (modeNormal);
		m_mainwin->setGameMode (modeNormal);
		break;

		// teach tools + game tree
	case tabTeachGameTree:
		break;

	default:
		break;
	}
}

// set a tab on toolsTabWidget
void MainWidget::setToolsTabWidget(enum tabType p, enum tabState s)
{
	QWidget *w = NULL;

	switch (p)
	{
		case tabNormalScore:
			w = tab_ns;
			break;

		case tabTeachGameTree:
			w = tab_tg;
			break;

		default:
			return;
	}

	if (s == tabSet)
	{
		// check whether the page to switch to is enabled
		if (!toolsTabWidget->isEnabled())
			toolsTabWidget->setTabEnabled(toolsTabWidget->indexOf (w), true);

		toolsTabWidget->setCurrentIndex(p);
	}
	else
	{
		// check whether the current page is to disable; then set to 'normal'
		if (s == tabDisable && toolsTabWidget->currentIndex() == p)
			toolsTabWidget->setCurrentIndex(tabNormalScore);

		toolsTabWidget->setTabEnabled(toolsTabWidget->indexOf (w), s == tabEnable);
	}
}

void MainWidget::on_colorButton_clicked (bool)
{
	stone_color col = gfx_board->swap_edit_to_move ();
	colorButton->setChecked (col == white);
}

void MainWidget::doRealScore (bool toggle)
{
	qDebug("MainWidget::doRealScore()");
	/* Can be called when the user toggles the button, but also when a GTP board
	   enters scoring after two passes.  */
	scoreButton->setChecked (toggle);
	if (toggle)
	{
		m_remember_mode = gfx_board->getGameMode();
		m_remember_tab = toolsTabWidget->currentIndex();

		setToolsTabWidget(tabTeachGameTree, tabDisable);

		m_mainwin->setGameMode (modeScore);
	}
	else
	{
		setToolsTabWidget(tabTeachGameTree, tabEnable);

		m_mainwin->setGameMode (m_remember_mode);
		setToolsTabWidget(static_cast<tabType>(m_remember_tab));
	}
}

/* The "Edit Position" button: Switch to edit mode.  */
void MainWidget::doEditPos (bool toggle)
{
	qDebug("MainWidget::doEditPos()");
	if (toggle)
		m_mainwin->setGameMode (modeEdit);
	else
		m_mainwin->setGameMode (modeNormal);
}

/* The "Edit Game" button: Open a new window to edit observed game.  */
void MainWidget::doEdit()
{
	qDebug("MainWidget::doEdit()");
	m_mainwin->slot_editBoardInNewWindow(false);
}

void MainWidget::sliderChanged(int n)
{
	if (sliderSignalToggle)
		gfx_board->gotoNthMoveInVar(n);
}

void MainWidget::toggleSlider(bool b)
{
	if (showSlider == b)
		return;

	showSlider = b;

	if (b)
	{
		slider->show();
		sliderLeftLabel->show();
		sliderRightLabel->show();
	}
	else
	{
		slider->hide();
		sliderLeftLabel->hide();
		sliderRightLabel->hide();
	}
}

// Overwritten from QWidget
void MainWidget::updateFont ()
{
	QFont f (setting->fontStandard);
	f.setBold (true);
	scoreTools->totalBlack->setFont (f);
	scoreTools->totalWhite->setFont (f);

	setFont (setting->fontStandard);
	normalTools->wtimeView->update_font (setting->fontClocks);
	normalTools->btimeView->update_font (setting->fontClocks);
}

void MainWidget::setGameMode(GameMode mode)
{
	setToolsTabWidget(tabTeachGameTree, mode == modeTeach ? tabEnable : tabDisable);

	passButton->setVisible (mode != modeObserve);
	doneButton->setVisible (mode == modeScore || mode == modeScoreRemote);
	scoreButton->setVisible (mode == modeNormal || mode == modeEdit || mode == modeScore);
	undoButton->setVisible (mode == modeScoreRemote || mode == modeMatch || mode == modeComputer || mode == modeTeach);
	adjournButton->setVisible (mode == modeMatch || mode == modeTeach || mode == modeScoreRemote);
	resignButton->setVisible (mode == modeMatch || mode == modeComputer || mode == modeTeach || mode == modeScoreRemote);
	resignButton->setEnabled (mode != modeScoreRemote);
	refreshButton->setVisible (false);
	editButton->setVisible (mode == modeObserve);
	editPosButton->setVisible (mode == modeNormal || mode == modeEdit || mode == modeScore);
	editPosButton->setEnabled (mode == modeNormal || mode == modeEdit);
	colorButton->setEnabled (mode == modeEdit || mode == modeNormal);

	slider->setEnabled (mode == modeNormal);

	passButton->setEnabled (mode != modeScore && mode != modeScoreRemote);
	editButton->setEnabled (mode != modeScore);
	scoreButton->setEnabled (mode != modeEdit);

	gfx_board->setMode (mode);

	scoreTools->setVisible (mode == modeScore || mode == modeScoreRemote);
	normalTools->setVisible (mode != modeScore && mode != modeScoreRemote);

	if (mode == modeMatch || mode == modeTeach) {
		qDebug () << gfx_board->player_is (white) << " : " << gfx_board->player_is (black);
		QWidget *timeSelf = gfx_board->player_is (black) ? normalTools->btimeView : normalTools->wtimeView;
		QWidget *timeOther = gfx_board->player_is (black) ? normalTools->wtimeView : normalTools->btimeView;
#if 0
		switch (gsName)
		{
		case IGS:
			timeSelf->setToolTip(tr("remaining time / stones"));
			break;

		default:
			timeSelf->setToolTip(tr("click to pause/unpause the game"));
			break;
		}
#else
		timeSelf->setToolTip (tr("click to pause/unpause the game"));
#endif
		timeOther->setToolTip (tr("click to add 1 minute to your opponent's clock"));
	} else {
		normalTools->btimeView->setToolTip (tr ("Time remaining for this move"));
		normalTools->wtimeView->setToolTip (tr ("Time remaining for this move"));
	}

}

void MainWidget::setMoveData(const game_state &gs, const go_board &b, GameMode mode)
{
	int brothers = gs.n_siblings ();
	int sons = gs.n_children ();
	stone_color to_move = gs.to_move ();
	int move_nr = gs.move_number ();
	int var_nr = gs.var_number ();

	QString w_str = QObject::tr("W");
	QString b_str = QObject::tr("B");

	QString s (QObject::tr ("Move") + " ");
	s.append (QString::number (move_nr));
	if (move_nr == 0) {
		/* Nothing to add for the root.  */
	} else if (gs.was_pass_p ()) {
		s.append(" (" + (to_move == black ? w_str : b_str) + " ");
		s.append(" " + QObject::tr("Pass") + ")");
	} else if (!gs.was_edit_p ()) {
		int x = gs.get_move_x ();
		int y = gs.get_move_y ();
		s.append(" (");
		s.append((to_move == black ? w_str : b_str) + " ");
		s.append(QString(QChar(static_cast<const char>('A' + (x < 9 ? x : x + 1)))));
		s.append(QString::number (b.size () - y) + ")");
	}
	s.append (QObject::tr ("\nVariation ") + QString::number (var_nr)
		  + QObject::tr (" of ") + QString::number (1 + brothers) + "\n");
	s.append (QString::number (sons) + " ");
	if (sons == 1)
		s.append (QObject::tr("child position"));
	else
		s.append (QObject::tr("child positions"));
	moveNumLabel->setText(s);

	bool warn = false;
	if (gs.was_move_p () && gs.get_move_color () == to_move)
		warn = true;
	if (gs.root_node_p ()) {
		bool empty = b.position_empty_p ();
		bool b_to_move = to_move == black;
		warn |= empty != b_to_move;
	}
	turnWarning->setVisible (warn);
	colorButton->setChecked (to_move == white);
#if 0
	statusTurn->setText(" " + s.right (s.length() - 5) + " ");  // Without 'Move '
	statusNav->setText(" " + QString::number(brothers) + "/" + QString::number(sons));
#endif

	if (to_move == black)
		turnLabel->setText(QObject::tr ("Black to play"));
	else
		turnLabel->setText(QObject::tr ("White to play"));

	bool is_root_node = gs.root_node_p ();
	bool good_mode = mode == modeNormal || mode == modeObserve;
	goPrevButton->setEnabled (good_mode && !is_root_node);
	goNextButton->setEnabled (good_mode && sons > 0);
	goFirstButton->setEnabled (good_mode && !is_root_node);
	goLastButton->setEnabled (good_mode && sons > 0);
	prevCommentButton->setEnabled (good_mode && !is_root_node);
	nextCommentButton->setEnabled (good_mode && sons > 0);
	prevNumberButton->setEnabled (good_mode && !is_root_node);
	nextNumberButton->setEnabled (good_mode && sons > 0);

	scoreTools->setVisible (mode == modeScore || mode == modeScoreRemote || gs.was_score_p ());
	normalTools->setVisible (mode != modeScore && mode != modeScoreRemote && !gs.was_score_p ());

	// Update slider
	toggleSliderSignal (false);

	int mv = gs.active_var_max ();
	setSliderMax (mv);
	slider->setValue (move_nr);
	toggleSliderSignal (true);
}


void MainWidget::recalc_scores(const go_board &b)
{
	int caps_b, caps_w, score_b, score_w;
	b.get_scores (caps_b, caps_w, score_b, score_w);

	scoreTools->capturesWhite->setText(QString::number(caps_w));
	scoreTools->capturesBlack->setText(QString::number(caps_b));
	normalTools->capturesWhite->setText(QString::number(caps_w));
	normalTools->capturesBlack->setText(QString::number(caps_b));

	scoreTools->terrWhite->setText(QString::number(score_w));
	scoreTools->totalWhite->setText(QString::number(score_w + caps_w + m_game->komi ()));
	scoreTools->terrBlack->setText(QString::number(score_b));
	scoreTools->totalBlack->setText(QString::number(score_b + caps_b));

	double res = score_w + caps_w + m_game->komi () - score_b - caps_b;
	if (res < 0)
		scoreTools->result->setText ("B+" + QString::number (-res));
	else if (res == 0)
		scoreTools->result->setText ("Jigo");
	else
		scoreTools->result->setText ("W+" + QString::number (res));
}

void MainWidget::setSliderMax(int n)
{
	if (n < 0)
		n = 0;

	slider->setMaximum(n);
	sliderRightLabel->setText(QString::number(n));
}

void MainWidget::toggleSidebar(bool toggle)
{
	if (!toggle)
		toolsFrame->hide();
	else
		toolsFrame->show();
}

void MainWidget::setTimes(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones,
			  bool warn_black, bool warn_white, int timer_cnt)
{
	if (!btime.isEmpty())
	{
		if (bstones != QString("-1"))
			normalTools->btimeView->set_text(btime + " / " + bstones);
		else
			normalTools->btimeView->set_text(btime);
	}

	if (!wtime.isEmpty())
	{
		if (wstones != QString("-1"))
			normalTools->wtimeView->set_text(wtime + " / " + wstones);
		else
			normalTools->wtimeView->set_text(wtime);
	}

	// warn if I am within the last 10 seconds
	if (gfx_board->getGameMode() == modeMatch)
	{
		if (gfx_board->player_is (black))
		{
			normalTools->wtimeView->flash (false);
			normalTools->btimeView->flash (timer_cnt % 2 != 0 && warn_black);
			if (warn_black)
				qgo->playTimeSound();
		}
		else if (gfx_board->player_is (white) && warn_white)
		{
			normalTools->btimeView->flash (false);
			normalTools->wtimeView->flash (timer_cnt % 2 != 0 && warn_white);
			if (warn_white)
				qgo->playTimeSound();
		}
	} else {
		normalTools->btimeView->flash (false);
		normalTools->wtimeView->flash (false);
	}
}

#if 0
void MainWidget::setTimes(bool isBlacksTurn, float time, int stones)
{
	QString strTime;
	int seconds = (int)time;
	bool neg = seconds < 0;
	if (neg)
		seconds = -seconds;

	int h = seconds / 3600;
	seconds -= h*3600;
	int m = seconds / 60;
	int s = seconds - m*60;

	QString sec;

	// prevailling 0 for seconds
	if ((h || m) && s < 10)
		sec = "0" + QString::number(s);
	else
		sec = QString::number(s);

	if (h)
	{
		QString min;

		// prevailling 0 for minutes
		if (h && m < 10)
			min = "0" + QString::number(m);
		else
			min = QString::number(m);

		strTime = (neg ? "-" : "") + QString::number(h) + ":" + min + ":" + sec;
	}
	else
		strTime = (neg ? "-" : "") + QString::number(m) + ":" + sec;

	if (isBlacksTurn)
		setTimes(strTime, QString::number(stones), 0, 0, false, false, 0);
	else
		setTimes(0, 0, strTime, QString::number(stones), false, false, 0);
}
#endif

void MainWidget::set_eval (double eval)
{
	m_eval = eval;
	int w = evalView->width ();
	int h = evalView->height ();
	m_eval_canvas->setSceneRect (0, 0, w, h);
	m_eval_bar->setRect (0, 0, w, h * eval);
	m_eval_bar->setPos (0, 0);
	m_eval_canvas->update ();
}

void MainWidget::set_eval (const QString &move, double eval, stone_color to_move, int visits)
{
	evalView->setBackgroundBrush (QBrush (Qt::white));
	m_eval_bar->setBrush (QBrush (Qt::black));
	set_eval (to_move == black ? eval : 1 - eval);

	int winrate_for = setting->readIntEntry ("ANALYSIS_WINRATE");
	stone_color wr_swap_col = winrate_for == 0 ? white : winrate_for == 1 ? black : none;
	if (to_move == wr_swap_col)
		eval = 1 - eval;

	normalTools->primaryCoords->setText (move);
	normalTools->primaryWR->setText (QString::number (100 * eval, 'f', 1));
	normalTools->primaryVisits->setText (QString::number (visits));
	if (winrate_for == 0 || (winrate_for == 2 && to_move == black)) {
		normalTools->pWRLabel->setText (tr ("B Win %"));
		normalTools->sWRLabel->setText (tr ("B Win %"));
	} else {
		normalTools->pWRLabel->setText (tr ("W Win %"));
		normalTools->sWRLabel->setText (tr ("W Win %"));
	}
}

void MainWidget::set_2nd_eval (const QString &move, double eval, stone_color to_move, int visits)
{
	if (move.isEmpty ()) {
		normalTools->shownCoords->setText ("");
		normalTools->shownWR->setText ("");
		normalTools->shownVisits->setText ("");
	} else {
		int winrate_for = setting->readIntEntry ("ANALYSIS_WINRATE");
		stone_color wr_swap_col = winrate_for == 0 ? white : winrate_for == 1 ? black : none;
		if (to_move == wr_swap_col)
			eval = 1 - eval;
		normalTools->shownCoords->setText (move);
		normalTools->shownWR->setText (QString::number (100 * eval, 'f', 1));
		normalTools->shownVisits->setText (QString::number (visits));
	}
}

void MainWidget::grey_eval_bar ()
{
	evalView->setBackgroundBrush (QBrush (Qt::lightGray));
	m_eval_bar->setBrush (QBrush (Qt::darkGray));
}
