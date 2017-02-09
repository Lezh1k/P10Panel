#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QMediaPlayer>

namespace Ui {
  class MainWindow;
}

class QSerialPort;
class QSerialPortInfo;
class QStandardItemModel;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private:
  typedef void (MainWindow::*pf_state_changed)(void);

  Ui::MainWindow *ui;
  enum {MAIN_TIMER_INTERVAL = 1000};
  enum AS_STATE{ AS_IDLE = 0, AS_RUN, AS_LAST } m_state;
  pf_state_changed m_state_callbacks[AS_LAST];
  QString m_btn_text[AS_LAST];

  QTimer m_main_timer;
  QTimer m_rotate_timer;

  int m_time_full;
  int m_time_alarm;
  int m_time_rotate;

  QMediaPlayer m_player;
  QSerialPort* m_serial_port;
  QStandardItemModel* m_model_ports;

  void Start(void);
  void Stop(void);

  void change_controls_enabled_state();
  void adjust_font_size(void);
  void time_elapsed(void);
  void set_current_time_text(void);

  void send_to_p10();

private slots:
  void ChkLoop_Changed(bool checked);
  void BtnStartStop_Clicked(void);
  void MainTimer_Timeout(void);
  void RotateTimer_Timeout(void);
  void TimeEdit_TimeChanged(const QTime &);
  void CbPorts_IndexChanged(int ix);

  // QWidget interface
protected:
  virtual void resizeEvent(QResizeEvent *event);
};

#endif // MAINWINDOW_H