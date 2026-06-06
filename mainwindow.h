#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QSettings>
#include <QMap>
#include <QStringList>

class MainWindow : public QMainWindow {
    Q_OBJECT

    struct TypingLesson {
        QString title;
        QString fullText;
        QStringList lines;
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTimerTick();

private:
    void setupUI();
    QWidget* createStartPage();
    QWidget* createTrainingPage();
    QWidget* createResultsPage();

    void loadLessonsList();
    void addDefaultLessons();
    void loadLessonFromFile(const QString &filePath);

    void startTraining();
    void restartTraining();
    void returnToMain();
    void finishTraining();

    void resetSession();
    void updateTrainingDisplay();
    void updateMetrics();
    void saveSettings();
    void loadSettings();

    QString formatTime(int seconds) const;

    QMap<int, TypingLesson> lessons;
    int currentLessonIndex = 0;
    int lineIndex = 0;
    int charIndex = 0;

    QTimer *sessionTimer;
    int secondsElapsed = 0;
    int totalCharsTyped = 0;
    int correctCharsTyped = 0;

    QStackedWidget *stackedPages;

    QComboBox *comboLesson;
    QComboBox *comboSpeedMode;
    QLabel *labelLessonDescription;

    QTextEdit *textDisplay;

    QLabel *labelTimeValue;
    QLabel *labelSpeedValue;
    QLabel *labelAccuracyValue;

    QLabel *labelFinalTime;
    QLabel *labelFinalSpeed;
    QLabel *labelFinalAccuracy;

    QPushButton *buttonStartTraining;
    QPushButton *buttonRestartTraining;
    QPushButton *buttonReturnToMain;
};

#endif