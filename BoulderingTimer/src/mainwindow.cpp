#include "include/mainwindow.h"
#include "include/p10_controller.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDesktopWidget>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QThread>
#include <QApplication>
#include <QFileDialog>

static const char* time_formats[2] = {"mm:ss", "hh:mm"};
static const int   time_coeffs[2] = {1, 60};

static const int TX_BUFFER_SIZE = 4;
static char g_tx_buffer[TX_BUFFER_SIZE] = {11, 11, 11, 11};

QMediaPlayer MainWindow::m_player;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  m_current_time_format(TF_MIN_SEC),
  ui(new Ui::MainWindow),
  m_state(AS_IDLE),
  m_time_full(240*1000),
  m_time_alarm(60*1000),
  m_time_rotate(15*1000),
  m_serial_port(nullptr),
  m_model_ports(nullptr) {

  ui->setupUi(this);
  m_warning_file = QApplication::applicationDirPath() + QDir::separator() + "alarm.wav";
  m_start_file = QApplication::applicationDirPath() + QDir::separator() + "start.wav";
  m_stop_file  = QApplication::applicationDirPath() + QDir::separator() + "stop.wav";

  ui->m_le_start_sound_path->setText(m_start_file);
  ui->m_le_warning_sound_path->setText(m_warning_file);
  ui->m_le_stop_sound_path->setText(m_stop_file);

  ui->m_te_full_time->setTime(QTime::fromMSecsSinceStartOfDay(m_time_full));
  ui->m_te_ring_time->setTime(QTime::fromMSecsSinceStartOfDay(m_time_alarm));
  ui->m_te_rotate_time->setTime(QTime::fromMSecsSinceStartOfDay(m_time_rotate));

  m_state_callbacks[AS_IDLE] = &MainWindow::Start;
  m_state_callbacks[AS_RUN] = &MainWindow::Stop;
  m_btn_text[AS_IDLE] = "Start";
  m_btn_text[AS_RUN] = "Stop";

  m_main_timer.setInterval(MAIN_TIMER_INTERVAL);
  m_rotate_timer.setInterval(m_time_rotate);

  ui->m_lbl_current_time->setText(ui->m_te_full_time->time().toString(time_formats[m_current_time_format]));
  m_player.setVolume(100);  

  m_model_ports = new QStandardItemModel;
  QList<QSerialPortInfo> lst_ports = QSerialPortInfo::availablePorts();
  for (auto i : lst_ports) {
    QStandardItem* item = new QStandardItem(i.portName());
    m_model_ports->appendRow(item);
  }
  ui->m_cb_serial_port->setModel(m_model_ports);

  connect(ui->m_chk_loop, SIGNAL(toggled(bool)), this, SLOT(ChkLoop_Changed(bool)));
  connect(ui->m_btn_start_stop, SIGNAL(pressed()), this, SLOT(BtnStartStop_Clicked()));
  connect(&m_main_timer, SIGNAL(timeout()), this, SLOT(MainTimer_Timeout()));
  connect(ui->m_te_full_time, SIGNAL(timeChanged(QTime)), this, SLOT(TimeEdit_TimeChanged(QTime)));
  connect(&m_rotate_timer, SIGNAL(timeout()), this, SLOT(RotateTimer_Timeout()));
  connect(ui->m_cb_serial_port, SIGNAL(currentIndexChanged(int)), this, SLOT(CbPorts_IndexChanged(int)));
  connect(ui->m_rb_hr_min, SIGNAL(toggled(bool)), this, SLOT(RbHrMin_Toggled(bool)));
  connect(ui->m_rb_min_sec, SIGNAL(toggled(bool)), this, SLOT(RbMinSec_Toggled(bool)));

  connect(ui->m_btn_dlg_start_sound, SIGNAL(released()), this, SLOT(BtnStartDlg_Released()));
  connect(ui->m_btn_dlg_warning_sound, SIGNAL(released()), this, SLOT(BtnWarningDlg_Released()));
  connect(ui->m_btn_dlg_stop_sound, SIGNAL(released()), this, SLOT(BtnStopDlg_Released()));

  connect(ui->m_btn_play_start_sound, SIGNAL(released()), this, SLOT(BtnPlayStartSound_Released()));
  connect(ui->m_btn_play_warning_sound, SIGNAL(released()), this, SLOT(BtnPlayWarningSound_Released()));
  connect(ui->m_btn_play_stop_sound, SIGNAL(released()), this, SLOT(BtnPlayStopSound_Released()));

  if (QSerialPortInfo::availablePorts().size() > 0)
    CbPorts_IndexChanged(0);

  g_tx_buffer[TX_BUFFER_SIZE-1] = 1;
  set_current_time_text();
  ui->m_lbl_error->setVisible(false);

  //UGLY HACK N1. I don't know how to adjust font size.
  //so I'm waiting here 100ms for constuction finishing and
  //then label is resized by layout. So I need to adjust font
  //size in that moment. Whoever knows how to adjust font size
  //right before form is shown - please send me this info to
  //lezh1k.vohrer@gmail.com.

  MainWindowInitializer* mwi = new MainWindowInitializer;
  QThread *th = new QThread;
  connect(th, SIGNAL(started()), mwi, SLOT(start_initialization()));
  connect(mwi, SIGNAL(waiting_finished()), this, SLOT(adjust_font_size()));
  connect(mwi, SIGNAL(waiting_finished()), th, SLOT(quit()));
  connect(th, SIGNAL(finished()), mwi, SLOT(deleteLater()));
  connect(th, SIGNAL(finished()), th, SLOT(deleteLater()));
  mwi->moveToThread(th);
  th->start();
}
/////////////////////////////////////////////////////////////////////////

MainWindow::~MainWindow() {
  delete ui;
  if (m_model_ports) delete m_model_ports;
  if (m_serial_port) delete m_serial_port;
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  adjust_font_size();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::Start() {  
  m_current_time_format = ui->m_rb_min_sec->isChecked() ? TF_MIN_SEC : TF_HR_MIN;
  play_media(m_start_file);
  m_rotate_timer.stop();
  m_time_full = ui->m_te_full_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];
  m_time_alarm = ui->m_te_ring_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];
  m_time_rotate = ui->m_te_rotate_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];
  m_rotate_timer.setInterval(m_time_rotate);
  set_current_time_text();
  m_main_timer.start();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::Stop() {  
  m_rotate_timer.stop();
  m_main_timer.stop();
  m_current_time_format = ui->m_rb_min_sec->isChecked() ? TF_MIN_SEC : TF_HR_MIN;

  m_time_full = ui->m_te_full_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];
  m_time_alarm = ui->m_te_ring_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];
  m_time_rotate = ui->m_te_rotate_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];

  set_current_time_text();
}
//////////////////////////////////////////////////////////////

void
MainWindow::change_controls_enabled_state() {
  bool ce = m_state == AS_IDLE; //controls enabled
  ui->m_te_full_time->setEnabled(ce);
  ui->m_te_ring_time->setEnabled(ce);
  ui->m_te_rotate_time->setEnabled(ce && ui->m_chk_loop->isChecked());
  ui->m_chk_loop->setEnabled(ce);
  ui->m_cb_serial_port->setEnabled(ce);
  ui->m_rb_hr_min->setEnabled(ce);
  ui->m_rb_min_sec->setEnabled(ce);
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::BtnStartStop_Clicked() {
  AS_STATE st = m_state;
  m_state = (AS_STATE)((st+1)%(AS_LAST));
  ui->m_btn_start_stop->setText(m_btn_text[m_state]);
  change_controls_enabled_state();
  (this->*m_state_callbacks[st])();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::time_elapsed() {  

  play_media(m_stop_file);
  m_main_timer.stop();
  m_current_time_format = ui->m_rb_min_sec->isChecked() ? TF_MIN_SEC : TF_HR_MIN;
  if (!ui->m_chk_loop->isChecked()) {
    BtnStartStop_Clicked();
    return;
  }
  m_rotate_timer.start();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::set_current_time_text() {
  QString time_str =
      QTime::fromMSecsSinceStartOfDay(m_time_full).toString(time_formats[m_current_time_format]);
  static const int indexes[TX_BUFFER_SIZE] = {0, 1, 3, 4};
  for (int i = 0; i < TX_BUFFER_SIZE; ++i) {
    if (g_tx_buffer[i] != time_str[indexes[i]].digitValue()) {
      g_tx_buffer[i] = time_str[indexes[i]].digitValue();
      CP10Controller::Instance()->set_digit(i, g_tx_buffer[i]);
    }
  }    
  ui->m_lbl_current_time->setText(time_str);  
}
//////////////////////////////////////////////////////////////

void
MainWindow::play_media(const QString &file_path) {
  m_player.stop();
  m_player.setMedia(QUrl::fromLocalFile(file_path));
  m_player.play();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::ChkLoop_Changed(bool checked) {
  ui->m_te_rotate_time->setEnabled(checked);
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::MainTimer_Timeout() {
  static const int hour = 60*60*1000;
  m_time_full -= MAIN_TIMER_INTERVAL;

  if (m_time_full < hour)
    m_current_time_format = TF_MIN_SEC;

  if (m_time_full == m_time_alarm)
    play_media(m_warning_file);


  if (m_time_full <= 0) {
    play_media(m_stop_file);
    time_elapsed();
  }
  set_current_time_text();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::RotateTimer_Timeout() {
  Start();
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::TimeEdit_TimeChanged(const QTime& ) {
  m_time_full = ui->m_te_full_time->time().msecsSinceStartOfDay() * time_coeffs[m_current_time_format];
  set_current_time_text();
}
//////////////////////////////////////////////////////////////

void MainWindow::CbPorts_IndexChanged(int ix) {
  if (m_serial_port) {
    delete m_serial_port;
    m_serial_port = nullptr;
  }

  ui->m_lbl_error->setVisible(false);
  QList<QSerialPortInfo> lst_ports = QSerialPortInfo::availablePorts();
  if (lst_ports.size() > ix) {
    m_serial_port = new QSerialPort(lst_ports.at(ix));
    m_serial_port->setBaudRate(9600);
    m_serial_port->setParity(QSerialPort::NoParity);
    m_serial_port->setDataBits(QSerialPort::Data8);
    m_serial_port->setStopBits(QSerialPort::OneStop);
    m_serial_port->setFlowControl(QSerialPort::NoFlowControl);
    if (!m_serial_port->open(QSerialPort::ReadWrite)) {
      ui->m_lbl_error->setVisible(true);
      ui->m_lbl_error->setText(m_serial_port->errorString());
      return;
    }
    CP10Controller::Instance()->set_serial_port(m_serial_port);
  } else {
    m_model_ports->clear();
    QList<QSerialPortInfo> lst_ports = QSerialPortInfo::availablePorts();
    for (auto i : lst_ports) {
      QStandardItem* item = new QStandardItem(i.portName());
      m_model_ports->appendRow(item);
    }
  }
}
//////////////////////////////////////////////////////////////

void
MainWindow::RbMinSec_Toggled(bool flag) {
  m_current_time_format = flag ? TF_MIN_SEC : TF_HR_MIN;
}
//////////////////////////////////////////////////////////////

void
MainWindow::RbHrMin_Toggled(bool flag) {
  m_current_time_format = flag ? TF_HR_MIN : TF_MIN_SEC;
}
//////////////////////////////////////////////////////////////

void
MainWindow::BtnStartDlg_Released() {
  QString fp = QFileDialog::getOpenFileName(nullptr, "Сигнал старта",
                                            QApplication::applicationDirPath(), "Wav files (*.wav);;");
  if (fp.isEmpty()) return;
  QFileInfo fi(fp);
  if (!fi.exists()) return;

  m_start_file = fp;
  ui->m_le_start_sound_path->setText(fp);
}
//////////////////////////////////////////////////////////////

void
MainWindow::BtnWarningDlg_Released() {
  QString fp = QFileDialog::getOpenFileName(nullptr, "Сигнал предупреждения",
                                            QApplication::applicationDirPath(), "Wav files (*.wav);;");
  if (fp.isEmpty()) return;
  QFileInfo fi(fp);
  if (!fi.exists()) return;

  m_warning_file = fp;
  ui->m_le_warning_sound_path->setText(fp);
}
//////////////////////////////////////////////////////////////

void
MainWindow::BtnStopDlg_Released() {
  QString fp = QFileDialog::getOpenFileName(nullptr, "Сигнал завершения",
                                            QApplication::applicationDirPath(), "Wav files (*.wav);;");
  if (fp.isEmpty()) return;
  QFileInfo fi(fp);
  if (!fi.exists()) return;

  m_stop_file = fp;
  ui->m_le_stop_sound_path->setText(fp);
}
//////////////////////////////////////////////////////////////

void
MainWindow::BtnPlayStartSound_Released() {
  play_media(m_start_file);
}
//////////////////////////////////////////////////////////////

void
MainWindow::BtnPlayWarningSound_Released() {
  play_media(m_warning_file);
}
//////////////////////////////////////////////////////////////

void
MainWindow::BtnPlayStopSound_Released() {
  play_media(m_stop_file);
}
/////////////////////////////////////////////////////////////////////////

void
MainWindow::adjust_font_size() {
  QString str = ui->m_lbl_current_time->text();
  QFont font = ui->m_lbl_current_time->font();
  QFontMetrics fm(font);  

  int f, l, m;
  l = 2048; f = 0; //we don't need such a big value. but I want to be sure that we will find font size.
  while (f < l) {
    m = (f+l) >> 1;
    font.setPointSize(m);
    fm = QFontMetrics(font);
    if (fm.height() > ui->m_lbl_current_time->height() ||
        fm.width(str) > ui->m_lbl_current_time->width()) {
      l = m-1;
    } else {
      f = m+1;
    }
  }
  font.setPointSize(l);
  ui->m_lbl_current_time->setFont(font);  
}
/////////////////////////////////////////////////////////////////////////

MainWindowInitializer::MainWindowInitializer() {

}

MainWindowInitializer::~MainWindowInitializer() {

}
////////////////////////////////////////////////////////

void
MainWindowInitializer::start_initialization() {
  QThread::msleep(100);
  emit waiting_finished();
}
/////////////////////////////////////////////////////////////////////////
