#ifndef X_GUI_H
#define X_GUI_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QWheelEvent>
#include <QKeyEvent>

// common

void shitHappens(const char*);
bool areSure(const char*);
int askYNC(const char*);
void showInfo(const char*);

// subclasses

#define	XHS_BGR	1		// change background if value changed
#define	XHS_DEC	(1<<1)		// hex/dec switch enabled

class xHexSpin : public QLineEdit {
	Q_OBJECT
	public:
		xHexSpin(QWidget* = NULL);
		void setValue(int);
		int getValue();
		void setMin(int);
		void setMax(int);
		void setXFlag(int);
	signals:
		void valueChanged(int);
	private slots:
		void onChange(int);
		void onTextChange(QString);
	private:
		int hsflag;
		int base;
		int value;
		int min;
		int max;
		void setBase(int);
		QString getText(int);
	protected:
		void keyPressEvent(QKeyEvent*);
		void wheelEvent(QWheelEvent*);
};

class xLabel : public QLabel {
	Q_OBJECT
	public:
		xLabel(QWidget* p = NULL);
		int id;
	signals:
		void clicked(QMouseEvent*);
	protected:
		void mousePressEvent(QMouseEvent*);
};

// tape player

#include "xgui/options/opt_tapecat.h"
#include "ui_tapewin.h"
#include "libxpeccy/tape.h"

enum {
	TW_STATE = 0,
	TW_REWIND,
	TW_BREAK
};

enum {
	TWS_PLAY = 1,
	TWS_REC,
	TWS_STOP,
	TWS_OPEN,
	TWS_REWIND
};

class TapeWin : public QDialog {
	Q_OBJECT
	public:
		TapeWin(QWidget*);
		//void drawStops(Tape*);
		//void setCheck(int);
		//void setProgress(int,int);
	public slots:
		void updProgress(Tape*);
		void upd(Tape*);
	private:
		Ui::TapeWin ui;
		int state;
	private slots:
		void doPlay();
		void doRec();
		void doStop();
		void doLoad();
		void doDClick(QModelIndex);
		void doClick(QModelIndex);
};

// rzx player

#include "ui_rzxplayer.h"
#include "libxpeccy/spectrum.h"

enum {
	RWS_PLAY = 1,
	RWS_STOP,
	RWS_PAUSE,
	RWS_OPEN
};

class RZXWin : public QDialog {
	Q_OBJECT
	public:
		RZXWin(QWidget*);
		void setProgress(int,int);
	public slots:
		void startPlay();
		void stop();
		void upd(Computer*);
	signals:
		void stateChanged(int);
	private:
		Ui::rzxPlayer ui;
		int state;
	private slots:
		void playPause();
		void open();
};

#endif
